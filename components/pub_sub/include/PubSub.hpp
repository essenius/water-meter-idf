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

    struct IntCoordinate {
        uint16_t x;
        uint16_t y;

        IntCoordinate() : x(0), y(0) {}
        IntCoordinate(int16_t xIn, int16_t yIn) : x(xIn), y(yIn) {}

        // we need this to pass coordinates in a memory efficient way but still keep reasonable accuracy
        static IntCoordinate times10(const double xIn, const double yIn) {
            return {static_cast<int16_t>(xIn * 10), static_cast<int16_t>(yIn * 10)};
        }

        friend std::ostream& operator<<(std::ostream& os, const IntCoordinate& coord) {
            os << "(" << coord.x << ", " << coord.y << ")";
            return os;
        }
    };
    
    using Payload = std::variant<int, float, const char*, IntCoordinate>;

    enum class Topic : uint8_t {
        None = 0,
        Anomaly,
        Drifted,
        NoFit,
        Pulse,
        Sample,
        SensorWasReset,
        AllTopics = UINT8_MAX
    };
    
        constexpr const char* toCString(Topic topic) {
        switch (topic) {
            case Topic::None: return "None";
            case Topic::Anomaly: return "Anomaly";
            case Topic::Drifted: return "Drifted";
            case Topic::NoFit: return "NoFit";
            case Topic::Pulse: return "Pulse";
            case Topic::Sample: return "Sample";
            case Topic::SensorWasReset: return "SensorWasReset";
            case Topic::AllTopics: return "AllTopics";
            default: return "Unknown";
        }
    }

    class Subscriber {
    public:
        virtual ~Subscriber() = default;
        virtual void subscriberCallback(const Topic topic, const Payload& payload) = 0;
    };

    using SubscriberHandle = Subscriber*;

    struct SubscriberMap {
        Topic topic;
        std::vector<SubscriberHandle> subscribers;
    };

    struct Message {
        SubscriberHandle source;
        Payload message;
        Topic topic;
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

            void operator()(const IntCoordinate& value) const {
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

        void publish(Topic topic, const Payload& message, const SubscriberHandle source = nullptr);
        void subscribe(const SubscriberHandle subscriber, Topic topic);
        void unsubscribe(const SubscriberHandle subscriber, Topic topic = Topic::AllTopics);
        void unsubscribeAll();
        bool isIdle() const;
        void receive();
        void waitForIdle() const;
        void dump_subscribers(const char* tag = "dump") const;

    private:
        bool addSubscriberToExistingTopic(SubscriberMap& subscriberMap, Topic topic, const SubscriberHandle subscriber);
        void callSubscriber(const SubscriberMap &subscriberMap, const Message &msg) const;
        bool doesCallbackExist(const SubscriberMap& subscriberMap, const SubscriberHandle subscriber) const;
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
        void removeSubscriber(const SubscriberHandle subscriber, SubscriberMap &subscriberMap, std::vector<SubscriberMap*>& mapsToClear);
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