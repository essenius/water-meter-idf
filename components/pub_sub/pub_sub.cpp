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

#include "pub_sub.hpp"
#include <algorithm>
#include "freertos/semphr.h"
#include <sstream>

namespace pubsub {

    const char* PubSub::TAG = "PubSub";

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
        ESP_LOGI(TAG, "Destructor for PubSub");
        unsubscribeAll();
        if (m_message_queue != nullptr) {
            vQueueDelete(m_message_queue);
        }
        if (m_eventLoopTaskHandle != nullptr) {
            m_terminateFlag.store(true);
            vTaskDelay(pdMS_TO_TICKS(10)); // Give some time for the task to check the flag and exit
            vTaskDelete(m_eventLoopTaskHandle);
        }
        ESP_LOGI(TAG, "Destructor for PubSub complete");
    }

    void PubSub::publish(uint16_t topic, const Payload& message, SubscriberCallback source) {
        Message msg;
        msg.topic = topic;
        msg.message = message;
        msg.source = source;
        char buffer[100];
        constexpr const char* TAG = "publish";
        std::visit(MessageVisitor(buffer), message);

        doInMutex(
            [this, msg, buffer]() -> bool {
                ESP_LOGI(TAG, "Publishing topic %d, message '%s'", msg.topic, buffer);
                if (xQueueSend(m_message_queue, &msg, portMAX_DELAY) != pdPASS) {
                    throwRuntimeError("PublishTopic", "Failed to publish topic " + std::to_string(msg.topic) + " message: " + buffer);
                }
                m_processing = true;
                return true;
            }, 
            "PublishTopic",
            std::to_string(topic).c_str()
        );
        ESP_LOGI(TAG, "Publish topic %d, message '%s' done", msg.topic, buffer);
    }

    bool PubSub::addSubscriberToExistingTopic(SubscriberMap& subscriberMap, uint16_t topic, SubscriberCallback callback) {
        // note: must be called with the mutex taken
        if (subscriberMap.topic != topic) return false;
        if (doesCallbackExist(subscriberMap, callback)) {
            ESP_LOGI(TAG, "Subscriber already exists for %d", topic);
            return true; // do not add it again
        }
        ESP_LOGI(TAG, "Adding subscriber to %d", topic);

        return doInMutex(
            [&subscriberMap, &callback]() {
                subscriberMap.subscribers.push_back(callback);
                return true;
            }, 
            "addSubscriberToExistingTopic", 
            std::to_string(topic).c_str()
        );
        return true;        
    }

    void PubSub::subscribe(uint16_t topic, SubscriberCallback callback) {
        constexpr const char* TAG = "subscribe";

        ESP_LOGI(TAG, "Subscribing to %d", topic);
        for (auto& subscriberMap: m_subscribers) {
            if (addSubscriberToExistingTopic(subscriberMap, topic, callback)) return;
        }
        // New topic, add to the map
        ESP_LOGI(TAG, "Adding subscriber map for %d", topic);
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
        subscriber.subscribers.push_back(callback);
        m_subscribers.push_back(subscriber);
        xSemaphoreGive(m_mutex);

    }

    // make sure the mutex is taken before this runs
    void PubSub::removeSubscriber(SubscriberCallback callback, SubscriberMap& subscriberMap, std::vector<SubscriberMap*>& mapsToClear) {
        dump_subscribers("removeSubscriber before");

        constexpr const char* TAG = "removeSubscriber";
        const uint16_t topic = subscriberMap.topic;
        ESP_LOGI(TAG, "Removing subscriber for %d, %p", topic, static_cast<void*>(callback.get()));

        // Manually iterate and remove subscribers that reference the specified callback
        for (auto it = subscriberMap.subscribers.begin(); it != subscriberMap.subscribers.end(); ++it ) {
            ESP_LOGI(TAG, "Comparing %p to %p", static_cast<void*>(it->get()), static_cast<void*>(callback.get()));
            if (static_cast<void*>(it->get()) == static_cast<void*>(callback.get())) {
                ESP_LOGI(TAG, "Removing subscriber for %d, %p, size %d", topic, static_cast<void*>(callback.get()), subscriberMap.subscribers.size());
                it = subscriberMap.subscribers.erase(it);
                break;
            }
        }

        if (subscriberMap.subscribers.empty()) {
            mapsToClear.push_back(&subscriberMap);
        }
        // Log the size of the subscribers vector after removal
        ESP_LOGI(TAG, "Size of subscribers after removal: %zu", subscriberMap.subscribers.size());
        dump_subscribers("removeSubscriber after");
    }

    void PubSub::throwRuntimeError(const std::string &context, const std::string &detail) const
    {
        std::stringstream ss;
        ss << context << ": " << detail;
        ESP_LOGE(TAG, "Throwing runtime error: %s", ss.str().c_str());
        for (;;);
    }

    void PubSub::removeMapsToClear(std::vector<SubscriberMap*>& mapsToClear) {
        // remove all maps to clear from m_subscribers
        for (auto* map : mapsToClear) {
            ESP_LOGI(TAG, "Removing map for %d", map->topic);
            m_subscribers.erase(
                std::remove_if(
                    m_subscribers.begin(),
                    m_subscribers.end(),
                    [map](const SubscriberMap& sm) { return sm.topic == map->topic; }),
                m_subscribers.end()
            );
        }
    }

    void PubSub::unsubscribe(uint16_t topic, SubscriberCallback callback) {
        ESP_LOGI(TAG, "Waiting to unsubscribe(%d, %p)", topic, static_cast<void*>(callback.get()));

        doInMutex(
            [this, topic, callback]() {
                ESP_LOGI(TAG, "Unsubscribing %d, %p", topic, static_cast<void*>(callback.get()));

                std::vector<SubscriberMap*> mapsToClear;
                for (auto& subscriberMap : m_subscribers) {
                    if (subscriberMap.topic == topic) {
                        removeSubscriber(callback, subscriberMap, mapsToClear);
                    }
                }
                ESP_LOGI(TAG, "Found %d maps to clear", mapsToClear.size());
                removeMapsToClear(mapsToClear);

                ESP_LOGI(TAG, "%d maps left", m_subscribers.size());
                return true;
            }, 
            "unsubscribe", 
            std::to_string(topic).c_str()
        );
    }

    void PubSub::unsubscribeAll() {
        ESP_LOGI(TAG, "Unsubscribing all (no-op)");
        /*for (auto& subscriberMap: m_subscribers) {
            subscriberMap.subscribers.clear();
        }
        m_subscribers.clear(); */
    }

    // unsubscribe a subscriber from all topics
    void PubSub::unsubscribe(SubscriberCallback callback) {
        dump_subscribers("unsubscribe(callback) before");
        ESP_LOGI(TAG, "Waiting to unsubscribe callback %p", static_cast<void*>(callback.get()));

        doInMutex(
            [this, callback]() {
                ESP_LOGI(TAG, "Unsubscribing callback %p", static_cast<void*>(callback.get()));

                std::vector<SubscriberMap*> mapsToClear;

                for (auto& subscriberMap : m_subscribers) {
                    removeSubscriber(callback, subscriberMap, mapsToClear);
                }

                ESP_LOGI(TAG, "Found %d maps to clear", mapsToClear.size());
                // remove all maps to clear from m_subscribers
                removeMapsToClear(mapsToClear);

                dump_subscribers("unsubscribe(callback) after");
                ESP_LOGI(TAG, "Done unsubscribe(callback %p)", static_cast<void*>(callback.get()));
                return true;
            }, 
            "unsubscribe", 
            "all"
        );
    }

    bool PubSub::isIdle() const {
        return uxQueueMessagesWaiting(m_message_queue) == 0 && !m_processing;
    }

    void PubSub::waitForIdle() const {
        constexpr const char* TAG = "waitForIdle";
        ESP_LOGI(TAG, "Waiting for idle... (%d messages waiting)", uxQueueMessagesWaiting(m_message_queue));
        while (!isIdle()) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        ESP_LOGI(TAG, "Done waiting for idle.");
    }

    // Private methods
    
    void PubSub::callSubscriber(const SubscriberMap &subscriberMap, const Message& msg) const {
        for (const auto &subscriber : subscriberMap.subscribers) {
            if (msg.source == nullptr || msg.source != subscriber) {
                (*subscriber)(msg.topic, msg.message);
            }
        }
    }

    bool PubSub::doesCallbackExist(const SubscriberMap& subscriberMap, SubscriberCallback callback) const {
        for (const auto& existingCallback : subscriberMap.subscribers) {
            if (existingCallback == callback) {
                return true;
            }
        }
        return false;
    }

    void PubSub::eventLoop(void* pubsubInstance) { 
        constexpr const char* TAG = "eventLoop";
        ESP_LOGI(TAG, "Starting event loop");
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
        constexpr const char* TAG = "receive";
        Message msg;
        ESP_LOGI(TAG, "Waiting to receive message");
        // not using doInMutex here, as we don't want to block the event loop
        if (xSemaphoreTake(m_mutex, 2* MutexTimeout) == pdTRUE) {
            ESP_LOGI(TAG, "Receiving message");
            if (xQueueReceive(m_message_queue, &msg, 0) == pdFAIL) {
                // nothing waiting in the queue
                m_processing = false;
                xSemaphoreGive(m_mutex);                
                ESP_LOGI(TAG, "No message received.");               
                vTaskDelay(pdMS_TO_TICKS(10)); 
            } else {
                m_processing = true;
                char buffer[100];
                std::visit(MessageVisitor(buffer), msg.message);
                xSemaphoreGive(m_mutex);                
                ESP_LOGI(TAG, "Receive: processing topic %d, message '%s'", msg.topic, buffer);	
                processMessage(msg);
                m_processing = false;
            }
        } 
    }

    void PubSub::processMessage(const Message& msg) const {
        constexpr const char* TAG = "processMessage";
        for (auto const& subscriberMap: m_subscribers) {
            if (subscriberMap.topic == msg.topic) {
                ESP_LOGI(TAG, "Calling subscriber for topic %d", msg.topic);
                callSubscriber(subscriberMap, msg);
                break;
            }
        }
    }

    void PubSub::dump_subscribers(const char* tag) const {
        ESP_LOGI(TAG, "Dumping subscribers (tag %s)", tag);
        for (auto const& subscriberMap: m_subscribers) {
            ESP_LOGI(TAG, "Topic %d", subscriberMap.topic);
            for (auto const& subscriber: subscriberMap.subscribers) {
                ESP_LOGI(TAG, "  Subscriber %p", subscriber.get());
            }
        }
    }
}