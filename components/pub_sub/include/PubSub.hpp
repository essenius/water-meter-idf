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

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <variant>
#include <functional>
#include <memory>
#include <format>
#include <atomic>
#include <cstring>


namespace pub_sub {

    constexpr uint16_t AllTopics = UINT16_MAX;

    struct Coordinate {
        uint16_t x;
        uint16_t y;

        friend std::ostream& operator<<(std::ostream& os, const Coordinate& coord) {
            os << "(" << coord.x << ", " << coord.y << ")";
            return os;
        }
    };
    using Payload = std::variant<int, float, const char*, Coordinate>;
    using Topic = uint16_t;
    using SubscriberCallback = std::function<void(const Topic, const Payload&)>;
    using SubscriberCallbackHandle = SubscriberCallback*;
    
    struct SubscriberMap {
        uint16_t topic;
        std::vector<SubscriberCallbackHandle> subscribers;
    };

    struct Message {
        SubscriberCallbackHandle source;
        Payload message;
        uint16_t topic;
    };

    template <size_t BufferSize>
    class MessageVisitor {
        public:
            explicit MessageVisitor(char(&buffer)[BufferSize]) :m_buffer(buffer) {}
    
            void operator()(int value) const {
                snprintf(m_buffer, m_bufferSize, "%d", value);
            }

            void operator()(float value) const {
                snprintf(m_buffer, m_bufferSize, "%f", value);
            }

            void operator()(const char* value) const {
                strncpy(m_buffer, value, m_bufferSize - 1);
                m_buffer[m_bufferSize] = '\0';
            }

            void operator()(const Coordinate& value) const {
                snprintf(m_buffer, m_bufferSize - 1, "%d, %d", value.x, value.y);
            }

        private:
            char *m_buffer;
            size_t m_bufferSize = BufferSize;
    };

    class PubSub {
    public:
        PubSub();
        ~PubSub();

        void publish(uint16_t topic, const Payload& message, const SubscriberCallbackHandle& source = nullptr);
        void subscribe(const SubscriberCallbackHandle& callback, uint16_t topic);
        void unsubscribe(const SubscriberCallbackHandle& callback, uint16_t topic = AllTopics);
        void unsubscribeAll();
        bool isIdle() const;
        void receive();
        void waitForIdle() const;
        void dump_subscribers(const char* tag = "dump") const;

    private:
        bool addSubscriberToExistingTopic(SubscriberMap& subscriberMap, uint16_t topic, const SubscriberCallbackHandle& callback);
        void callSubscriber(const SubscriberMap &subscriberMap, const Message &msg) const;
        bool doesCallbackExist(const SubscriberMap& subscriberMap, const SubscriberCallbackHandle& callback) const;
        void removeMapsToClear(const std::vector<SubscriberMap *> &mapsToClear);

        template <typename Func>
        bool doInMutex(Func&& operation, const char* context, const char* detail) {
            if (xSemaphoreTake(m_mutex, MutexTimeout + 2) != pdTRUE) {
                throwRuntimeError(context, std::string("Failed to take semaphore for " + std::string(detail)));
            }
            bool result = operation();
            if (xSemaphoreGive(m_mutex) != pdTRUE) {
                throwRuntimeError(context, std::string("Failed to give semaphore for " + std::string(detail)));
            }
            return result;
        }

#ifdef ESP_PLATFORM
        [[noreturn]]
#endif
        static void eventLoop(void* pubsubInstance);
        void processMessage(const Message &msg) const;
        void removeSubscriber(const SubscriberCallbackHandle& callback, SubscriberMap &subscriberMap, std::vector<SubscriberMap*>& mapsToClear);
        [[noreturn]] void throwRuntimeError(const std::string& context, const std::string& detail) const;

        std::vector<SubscriberMap> m_subscribers;
        int m_topic_count;
        QueueHandle_t m_message_queue;
        SemaphoreHandle_t m_mutex;
        TaskHandle_t m_eventLoopTaskHandle;
        std::atomic<bool> m_terminateFlag;

        static const int MutexTimeout = pdMS_TO_TICKS(1000);

        std::condition_variable m_conditionVariable;
        bool m_processing;
    };

}