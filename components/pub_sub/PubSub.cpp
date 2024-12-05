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

#include "PubSub.hpp"
#include <algorithm>
#include "freertos/semphr.h"
#include <sstream>

namespace pub_sub {

    // Public contstructors and methods

    PubSub::PubSub() : m_topic_count(0), m_terminateFlag(false), m_processing(false) {
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
        std::shared_ptr<PubSub> instance(new PubSub());
        instance->begin();
        return instance;
    }

    PubSub::~PubSub()
    {
        unsubscribeAll();

        if (m_message_queue != nullptr) {
            vQueueDelete(m_message_queue);
        }
        if (m_eventLoopTaskHandle != nullptr) {
            m_terminateFlag.store(true);
            vTaskDelay(pdMS_TO_TICKS(10)); // Give some time for the task to check the flag and exit
            vTaskDelete(m_eventLoopTaskHandle);
        }
        if (m_mutex != nullptr) {
            vSemaphoreDelete(m_mutex);
        }
    }

    void PubSub::begin() {
        // Start the event loop task
        auto sharedThis = shared_from_this();
        if (xTaskCreate([](void* param) { eventLoop(std::weak_ptr<PubSub>(static_cast<PubSub*>(param)->shared_from_this())); }, 
                        "EventLoop", 16384, this, 3, &m_eventLoopTaskHandle) != pdPASS) {
            vQueueDelete(m_message_queue);
            vSemaphoreDelete(m_mutex);
            throwRuntimeError("PubSub", "Failed to create event loop task");
        }
    }

    void PubSub::publish(Topic topic, const Payload &message, const SubscriberHandle source)
    {

        constexpr const char* kTag = "publish";

        doInMutex(
            [this, topic, message, source]() {
                Message msg;
                msg.topic = topic;
                msg.message = message;
                msg.source = source;
                if (xQueueSend(m_message_queue, &msg, portMAX_DELAY) != pdPASS) {
                    char buffer[100];
                    MessageVisitor visitor(buffer);
                    std::visit(visitor, message);
                    throwRuntimeError(kTag, std::string("Failed to publish topic ") + toCString(topic) + ", message: " + buffer);
                }
                m_processing = true;
                return true;
            }, 
            kTag,
            toCString(topic)
        );
    }

    bool PubSub::addSubscriberToExistingTopic(SubscriberMap& subscriberMap, Topic topic, const SubscriberHandle subscriber) {
        // note: must be called with the mutex taken
        constexpr const char* kTag = "addSubscriberToExistingTopic";
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
        return true;        
    }

    void PubSub::subscribe(const SubscriberHandle subscriber, Topic topic) {
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

    // expects the mutex to be taken
    void PubSub::removeSubscriber(const SubscriberHandle subscriber, SubscriberMap& subscriberMap, std::vector<SubscriberMap*>& mapsToClear) {
        // Manually iterate and remove subscribers that reference the specified callback
        for (auto it = subscriberMap.subscribers.begin(); it != subscriberMap.subscribers.end(); ++it ) {
            if (static_cast<void*>(*it) == static_cast<void*>(subscriber)) {
                it = subscriberMap.subscribers.erase(it);
                break;
            }
        }

        if (subscriberMap.subscribers.empty()) {
            mapsToClear.push_back(&subscriberMap);
        }
    }

    [[noreturn]] void PubSub::throwRuntimeError(const std::string &context, const std::string &detail) const {
        std::stringstream ss;
        ss << context << ": " << detail;
        ESP_LOGE("PubSub", "Throwing runtime error: %s", ss.str().c_str());
        for (;;);
    }

    // expects the mutex to be taken
    void PubSub::removeMapsToClear(const std::vector<SubscriberMap*>& mapsToClear) {
        // remove all maps to clear from m_subscribers
        for (auto* map : mapsToClear) {
            m_subscribers.erase(
                std::remove_if(
                    m_subscribers.begin(),
                    m_subscribers.end(),
                    [map](const SubscriberMap& sm) { return sm.topic == map->topic; }),
                m_subscribers.end()
            );
        }
    }

    void PubSub::unsubscribe(const SubscriberHandle subscriber, Topic topic) {
        constexpr const char* kTag = "unsubscribe";

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
        dump_subscribers("unsubscribeAll before");
        doInMutex(
            [this]() {
                for (auto& subscriberMap: m_subscribers) {
                    subscriberMap.subscribers.clear();
                }
                m_subscribers.clear();
                return true;
            }, 
            "hello", 
            "hello"
        );

        dump_subscribers("unsubscribeAll after");
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
    
    void PubSub::callSubscriber(const SubscriberMap &subscriberMap, const Message& msg) const {
        for (const auto &subscriber : subscriberMap.subscribers) {
            if (msg.source == nullptr || msg.source != subscriber) {
                subscriber->subscriberCallback(msg.topic, msg.message);
            }
        }
    }

    bool PubSub::doesCallbackExist(const SubscriberMap& subscriberMap, const SubscriberHandle subscriber) const {
        for (const auto& existingSubscriber : subscriberMap.subscribers) {
            if (existingSubscriber == subscriber) {
                return true;
            }
        }
        return false;
    }

    void PubSub::eventLoop(std::weak_ptr<PubSub> weakPubSub) { 
        while (true) {
            if (auto sharedPubSub = weakPubSub.lock()) {
                if (sharedPubSub->m_terminateFlag.load()) {
                    break;
                }
                sharedPubSub->receive();
            } else {
                // The PubSub instance has been deleted
                break;
            }
        }
    }

    void PubSub::receive() {
        Message msg;
        // not using doInMutex here, as we don't want to block the event loop
        if (xSemaphoreTake(m_mutex, 2* MutexTimeout) == pdTRUE) {
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

    void PubSub::processMessage(const Message& msg) const {
        for (auto const& subscriberMap: m_subscribers) {
            if (subscriberMap.topic == msg.topic) {
                callSubscriber(subscriberMap, msg);
                break;
            }
        }
    }

    void PubSub::dump_subscribers(const char* tag) const {
        constexpr const char* kTag = "dump_subscribers";
        ESP_LOGI(kTag, "Dumping subscribers (tag %s)", tag);
        for (auto const& subscriberMap: m_subscribers) {
            ESP_LOGI(kTag, "Topic %d", static_cast<uint8_t>(subscriberMap.topic));
            for (auto const& subscriber: subscriberMap.subscribers) {
                ESP_LOGI(kTag, "  Subscriber %p", subscriber);
            }
        }
    }


    void Subscriber::subscriberCallback(const Topic topic, const Payload &payload) {
        m_topic = topic;
        m_payload = payload;
    }
}
