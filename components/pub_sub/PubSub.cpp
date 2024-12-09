// Copyright 2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include <fstream>
#include "freertos/freeRTOS.h"
#include "freertos/semphr.h"
#include <sstream>
#include <algorithm>
#include "PubSub.hpp"
#include "esp_log.h"

namespace pub_sub {

    // Public constructors and methods

    PubSub::PubSub() : m_processing(false), m_terminateFlag(false)  {
        m_mutex = xSemaphoreCreateMutex();
        if (m_mutex == nullptr) {
            throwRuntimeError("PubSub", "Failed to create mutex");
        }
        xSemaphoreGive(m_mutex);

        m_message_queue = xQueueCreate(100, sizeof(Message));
        if (m_message_queue == nullptr) {
            throwRuntimeError("PubSub", "Failed to create message queue");
        }
    }

    std::shared_ptr<PubSub> PubSub::create() {
        auto instance = std::make_shared<PubSub>();
   		ESP_LOGI("create", "Reference count after make_shared: %ld", instance->getReferenceCount());

        instance->begin();
   		ESP_LOGI("create", "Reference count after begin: %ld", instance->getReferenceCount());
        return instance;
    }

    PubSub::~PubSub() {
        ESP_LOGI("~PubSub", "Destroying pubsub");
        unsubscribeAll();
        end();
        ESP_LOGI("~PubSub", "Deleting queue and mutex");
        if (m_message_queue != nullptr) {
            vQueueDelete(m_message_queue);
        }
        if (m_mutex != nullptr) {
            vSemaphoreDelete(m_mutex);
        }
        ESP_LOGI("~PubSub", "Done destroying pubsub");
    }

    void PubSub::begin() {
        // Start the event loop task

        m_eventLoopFinished.store(false);
        ESP_LOGI("begin", "Reference count after defining self: %ld", getReferenceCount());
        if (xTaskCreate(eventLoopTask, "EventLoop", 16384, this, 3, & m_eventLoopTaskHandle) != pdPASS) {
            vQueueDelete(m_message_queue);
            vSemaphoreDelete(m_mutex);
            throwRuntimeError("PubSub", "Failed to create event loop task");
        }
		ESP_LOGI("begin", "Reference count after creating task: %ld", getReferenceCount());
    }

    void PubSub::end() {
        // We want to clean out the shared pointer when we are done, so rather than just killing the task, we ask it to terminate
        // on ESP, it won't actually terminate but will keep running idle until vTaskDelete is called
        ESP_LOGI("end", "Terminating task");
        m_terminateFlag.store(true);

        while (!m_eventLoopFinished.load()) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        // the task deletes itself, so no need to do anything here
    }

    long PubSub::getReferenceCount() const {
        return shared_from_this().use_count();
    }

    void PubSub::publish(Topic topic, const Payload &message, const SubscriberHandle source) {
        constexpr auto kTag = "publish";

        doInMutex(
            [this, topic, message, source]() {
                Message msg;
                msg.topic = topic;
                msg.message = message;
                msg.source = source;
                if (xQueueSend(m_message_queue, &msg, portMAX_DELAY) != pdPASS) {
                    char buffer[100] = {};
                    MessageVisitor visitor(buffer);
                    std::visit(visitor, message);
                    buffer[99] = '\0';
                    throwRuntimeError(kTag, std::string("Failed to publish topic ") + toCString(topic) + ", message: " + buffer);
                }
                m_processing = true;
                return true;
            }, 
            kTag,
            toCString(topic)
        );
    }

    void PubSub::subscribe(const SubscriberHandle subscriber, const Topic topic) {
        for (auto& subscriberMap: m_subscribers) {
            if (addSubscriberToExistingTopic(subscriberMap, topic, subscriber)) return;
        }
        // New topic, add to the map
        SubscriberMap subscriberMap;
        subscriberMap.topic = topic;
        doInMutex(
            [this, &subscriberMap, subscriber]() {
                subscriberMap.subscribers.push_back(subscriber);
                m_subscribers.push_back(subscriberMap);
                return true;
            }, 
            "addSubscriberToNewTopic", 
            toCString(topic)
        );
    }

    void PubSub::unsubscribe(const SubscriberHandle subscriber, Topic topic) {
        constexpr auto kTag = "unsubscribe";

        doInMutex(
            [this, topic, subscriber]() {
                std::vector<SubscriberMap*> mapsToClear;
                for (auto& subscriberMap : m_subscribers) {
                    if (topic == Topic::AllTopics || subscriberMap.topic == topic) {
                        removeSubscriber(subscriber, subscriberMap, mapsToClear);
                    }
                }
                removeMapsToClear(mapsToClear);
                return true;
            }, 
            kTag, 
            toCString(topic)
        );
    }

    void PubSub::unsubscribeAll() {
        ESP_LOGI("UnsubscribeAll", "Unsubscribing all");
        dumpSubscribers("unsubscribeAll before");
        doInMutex(
            [this]() {
                for (auto& [topic, subscribers]: m_subscribers) {
                    subscribers.clear();
                }
                m_subscribers.clear();
                return true;
            }, 
            "hello", 
            "hello"
        );

        dumpSubscribers("unsubscribeAll after");
    }

    bool PubSub::isIdle() const {
        return uxQueueMessagesWaiting(m_message_queue) == 0 && !m_processing;
    }

    void PubSub::waitForIdle() const {
        while (!isIdle()) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    // Private methods

    bool PubSub::addSubscriberToExistingTopic(SubscriberMap& subscriberMap, const Topic topic, const SubscriberHandle subscriber) {
        // note: must be called with the mutex taken
        constexpr auto kTag = "addSubscriberToExistingTopic";
        if (subscriberMap.topic != topic) return false;
        if (doesCallbackExist(subscriberMap, subscriber)) {
            return true; // do not add it again
        }

        return doInMutex(
            [&subscriberMap, &subscriber]() {
                subscriberMap.subscribers.push_back(subscriber);
                return true;
            },
            kTag,
            toCString(topic)
        );
    }

    void PubSub::callSubscriber(const SubscriberMap &subscriberMap, const Message& msg) {
        for (const auto &subscriber : subscriberMap.subscribers) {
            if (msg.source == nullptr || msg.source != subscriber) {
                subscriber->subscriberCallback(msg.topic, msg.message);
            }
        }
    }

    bool PubSub::doesCallbackExist(const SubscriberMap& subscriberMap, const SubscriberHandle subscriber) {
        return std::ranges::any_of(subscriberMap.subscribers, [subscriber](const auto& existingSubscriber) {
            return existingSubscriber == subscriber;
        });
    }

    void PubSub::eventLoop(const std::shared_ptr<PubSub>& sharedPubSub) { 
        while (true) {
            if (sharedPubSub->m_terminateFlag.load()) {
                ESP_LOGI("eventLoop", "Terminating. Reference count: %ld", sharedPubSub->getReferenceCount());
                break;
            }
            sharedPubSub->receive();
            taskYIELD();
        }

        // Signal completion
        ESP_LOGI("eventLoop", "Signalling completion");
        sharedPubSub->m_eventLoopFinished.store(true);
    }

    void PubSub::eventLoopTask(void* param) {
        {
            // doing this in a block to ensure sharedPubSub is destroyed before the task is deleted
            auto sharedPubSub = static_cast<PubSub*>(param)->shared_from_this();
            sharedPubSub->eventLoop(sharedPubSub);
            ESP_LOGI("eventLoopTask", "Deleting task");
        }
        vTaskDelete(nullptr);
    }

    void PubSub::receive() {
        Message msg;
        // not using doInMutex here, as we want to release the mutex before processing/waiting
        if (xSemaphoreTake(m_mutex, 2* kMutexTimeout) == pdTRUE) {
            if (xQueueReceive(m_message_queue, &msg, 0) == pdFAIL) {
                // nothing waiting in the queue
                m_processing = false;
                xSemaphoreGive(m_mutex); 
                vTaskDelay(pdMS_TO_TICKS(10)); 
            } else {
                m_processing = true;
                char buffer[100];
                std::visit(MessageVisitor(buffer), msg.message);
                xSemaphoreGive(m_mutex);                
                processMessage(msg);
                m_processing = false;
            }
        } 
    }

    // expects the mutex to be taken before the operation
    void PubSub::removeMapsToClear(const std::vector<SubscriberMap*>& mapsToClear) {
        // remove all maps to clear from m_subscribers
        for (auto* map : mapsToClear) {
            std::erase_if(
                m_subscribers,
                [map](const SubscriberMap& sm) { return sm.topic == map->topic; });
        }
    }

    // expects the mutex to be taken
    void PubSub::removeSubscriber(const SubscriberHandle subscriber, SubscriberMap& subscriberMap, std::vector<SubscriberMap*>& mapsToClear) {
        // Manually iterate and remove subscribers that reference the specified callback
        for (auto it = subscriberMap.subscribers.begin(); it != subscriberMap.subscribers.end(); ++it) {
            if (static_cast<void*>(*it) == static_cast<void*>(subscriber)) {
                it = subscriberMap.subscribers.erase(it);
                break;
            }
        }

        if (subscriberMap.subscribers.empty()) {
            mapsToClear.push_back(&subscriberMap);
        }
    }
    void PubSub::processMessage(const Message& msg) const {
        for (auto const& subscriberMap: m_subscribers) {
            if (subscriberMap.topic == msg.topic) {
                callSubscriber(subscriberMap, msg);
                break;
            }
        }
    }

    void PubSub::dumpSubscribers(const char* tag) const {
        constexpr auto kTag = "dump_subscribers";
        ESP_LOGI(kTag, "Dumping subscribers (tag %s)", tag);
        for (const auto& [topic, subscribers]: m_subscribers) {
            ESP_LOGI(kTag, "Topic %d", static_cast<uint8_t>(topic));
            for (auto const& subscriber: subscribers) {
                ESP_LOGI(kTag, "  Subscriber %p", subscriber);
            }
        }
    }

    void Subscriber::subscriberCallback(const Topic topic, const Payload &payload) {
        m_topic = topic;
        m_payload = payload;
    }

    [[noreturn]] void PubSub::throwRuntimeError(const std::string& context, const std::string& detail) {
        std::stringstream ss;
        ss << context << ": " << detail;
        ESP_LOGE("PubSub", "Throwing runtime error: %s", ss.str().c_str());
        for (;;);
    }
}
