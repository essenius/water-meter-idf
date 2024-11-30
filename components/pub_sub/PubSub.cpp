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
        xSemaphoreGive(m_mutex);

        m_message_queue = xQueueCreate(100, sizeof(Message));
        if (m_message_queue == nullptr) {
            throwRuntimeError("PubSub", "Failed to create message queue");
        }
        
        // Start the event loop task
        xTaskCreate(eventLoop, "EventLoop", 4096, this, 3, &m_eventLoopTaskHandle);
    }

    PubSub::~PubSub() {
        unsubscribeAll();
        if (m_message_queue != nullptr) {
            vQueueDelete(m_message_queue);
        }
        if (m_eventLoopTaskHandle != nullptr) {
            m_terminateFlag.store(true);
            vTaskDelay(pdMS_TO_TICKS(10)); // Give some time for the task to check the flag and exit
            vTaskDelete(m_eventLoopTaskHandle);
        }
    }

    void PubSub::publish(uint16_t topic, const Payload& message, const SubscriberCallbackHandle& source) {

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
                    throwRuntimeError(kTag, "Failed to publish topic " + std::to_string(msg.topic) + ", message: " + buffer);
                }
                m_processing = true;
                return true;
            }, 
            kTag,
            std::to_string(topic).c_str()
        );
    }

    bool PubSub::addSubscriberToExistingTopic(SubscriberMap& subscriberMap, uint16_t topic, const SubscriberCallbackHandle& callback) {
        // note: must be called with the mutex taken
        constexpr const char* kTag = "addSubscriberToExistingTopic";
        if (subscriberMap.topic != topic) return false;
        if (doesCallbackExist(subscriberMap, callback)) {
            return true; // do not add it again
        }

        return doInMutex(
            [&subscriberMap, &callback]() {
                subscriberMap.subscribers.push_back(callback);
                return true;
            }, 
            kTag, 
            std::to_string(topic).c_str()
        );
        return true;        
    }

    void PubSub::subscribe(const SubscriberCallbackHandle& callback, uint16_t topic) {
        for (auto& subscriberMap: m_subscribers) {
            if (addSubscriberToExistingTopic(subscriberMap, topic, callback)) return;
        }
        // New topic, add to the map
        SubscriberMap subscriber;
        subscriber.topic = topic;
        doInMutex(
            [this, &subscriber, &callback]() {
                subscriber.subscribers.push_back(callback);
                m_subscribers.push_back(subscriber);
                return true;
            }, 
            "addSubscriberToNewTopic", 
            std::to_string(topic).c_str()
        );
    }

    // expects the mutex to be taken
    void PubSub::removeSubscriber(const SubscriberCallbackHandle& callback, SubscriberMap& subscriberMap, std::vector<SubscriberMap*>& mapsToClear) {
        // Manually iterate and remove subscribers that reference the specified callback
        for (auto it = subscriberMap.subscribers.begin(); it != subscriberMap.subscribers.end(); ++it ) {
            if (static_cast<void*>(*it) == static_cast<void*>(callback)) {
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

    void PubSub::unsubscribe(const SubscriberCallbackHandle& callback, uint16_t topic) {
        constexpr const char* kTag = "unsubscribe";

        doInMutex(
            [this, topic, callback]() {
                std::vector<SubscriberMap*> mapsToClear;
                for (auto& subscriberMap : m_subscribers) {
                    if (topic == AllTopics || subscriberMap.topic == topic) {
                        removeSubscriber(callback, subscriberMap, mapsToClear);
                    }
                }
                removeMapsToClear(mapsToClear);
                return true;
            }, 
            kTag, 
            std::to_string(topic).c_str()
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
                (*subscriber)(msg.topic, msg.message);
            }
        }
    }

    bool PubSub::doesCallbackExist(const SubscriberMap& subscriberMap, const SubscriberCallbackHandle& callback) const {
        for (const auto& existingCallback : subscriberMap.subscribers) {
            if (existingCallback == callback) {
                return true;
            }
        }
        return false;
    }

    void PubSub::eventLoop(void* pubsubInstance) { 
        const auto me = static_cast<PubSub*>(pubsubInstance);
#ifdef ESP_PLATFORM
        while(true) {
#else
        while (!me->m_terminateFlag.load()) {
#endif
            me->receive();
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
            ESP_LOGI(kTag, "Topic %d", subscriberMap.topic);
            for (auto const& subscriber: subscriberMap.subscribers) {
                ESP_LOGI(kTag, "  Subscriber %p", subscriber);
            }
        }
    }
}
