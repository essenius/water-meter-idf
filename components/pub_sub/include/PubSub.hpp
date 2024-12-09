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
#include <condition_variable>
#include <vector>
#include <variant>
#include <memory>
#include <format>
#include <atomic>
#include <cstring>
#include <iostream>

namespace pub_sub {

    struct IntCoordinate {
        int16_t x;
        int16_t y;

        IntCoordinate() : x(0), y(0) {}
        IntCoordinate(const int16_t xIn, const int16_t yIn) : x(xIn), y(yIn) {}

        // we need this to pass coordinates in a memory efficient way but still keep reasonable accuracy
        static IntCoordinate times10(const double xIn, const double yIn) {
            return {static_cast<int16_t>(xIn * 10), static_cast<int16_t>(yIn * 10)};
        }

        friend std::ostream& operator<<(std::ostream& os, const IntCoordinate& coordinate) {
            os << "(" << coordinate.x << ", " << coordinate.y << ")";
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
        Subscriber() = default;
        virtual ~Subscriber() = default;
        Subscriber(const Subscriber&) = delete;
        Subscriber& operator=(const Subscriber&) = delete;
        Subscriber(Subscriber&&) = delete;
        Subscriber& operator=(Subscriber&&) = delete;
        
        [[nodiscard]] Topic getTopic() const { return m_topic; }
            [[nodiscard]] Payload getPayload() const { return m_payload; }
            virtual void reset() { 
                m_topic = Topic::None; 
                m_payload = 0;
            }
            virtual void subscriberCallback(Topic topic, const Payload& payload);
        private:
            Topic m_topic = Topic::None;
            Payload m_payload = 0;
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
    
            void operator()(const int value) const {
                snprintf(m_buffer, m_bufferSize, "%d", value);
            }

            void operator()(const float value) const {
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
    
    class PubSub final : public std::enable_shared_from_this<PubSub> {
    public:
        PubSub();
        static std::shared_ptr<PubSub> create();
        ~PubSub();
        PubSub(const PubSub&) = delete;
        PubSub& operator=(const PubSub&) = delete;
        PubSub(PubSub&&) = delete;
        PubSub& operator=(PubSub&&) = delete;
        void begin();
        void end();
        bool isIdle() const;
        void publish(Topic topic, const Payload& message, SubscriberHandle source = nullptr);
        void receive();
        void subscribe(SubscriberHandle subscriber, Topic topic);
        void unsubscribe(SubscriberHandle subscriber, Topic topic = Topic::AllTopics);
        void unsubscribeAll();
        void waitForIdle() const;

        void dumpSubscribers(const char* tag = "dump") const;
        long getReferenceCount() const;

    private:
        bool addSubscriberToExistingTopic(SubscriberMap& subscriberMap, Topic topic, SubscriberHandle subscriber);
        static void callSubscriber(const SubscriberMap &subscriberMap, const Message &msg);
        static bool doesCallbackExist(const SubscriberMap& subscriberMap, SubscriberHandle subscriber);

        template <typename Func>
        bool doInMutex(Func&& operation, const char* context, const char* detail) {
            if (xSemaphoreTake(m_mutex, kMutexTimeout + 2) != pdTRUE) {
                throwRuntimeError(context, std::string("Failed to take semaphore for " + std::string(detail)));
            }
            const bool result = std::forward<Func>(operation)();
            if (xSemaphoreGive(m_mutex) != pdTRUE) {
                throwRuntimeError(context, std::string("Failed to give semaphore for " + std::string(detail)));
            }
            return result;
        }

        static void eventLoop(const std::shared_ptr<PubSub>& sharedPubSub);
        static void eventLoopTask(void* param);
        void processMessage(const Message &msg) const;
        void removeMapsToClear(const std::vector<SubscriberMap*>& mapsToClear);
        static void removeSubscriber(SubscriberHandle subscriber, SubscriberMap &subscriberMap, std::vector<SubscriberMap*>& mapsToClear);
        [[noreturn]] static void throwRuntimeError(const std::string& context, const std::string& detail);

        static constexpr int kMutexTimeout = pdMS_TO_TICKS(1000);

        std::atomic<bool> m_eventLoopFinished = true;
        TaskHandle_t m_eventLoopTaskHandle;
        SemaphoreHandle_t m_mutex;
        QueueHandle_t m_message_queue;
        bool m_processing;
        std::vector<SubscriberMap> m_subscribers;
        std::atomic<bool> m_terminateFlag;

    };

}