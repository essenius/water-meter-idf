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

#include <limits.h>
#include "unity.h"
#include "PubSub.hpp"
#include <atomic>
#include <thread>
#include <esp_log.h>
#include "TestPubSub.hpp"


namespace pub_sub {
    
    void customDeleter(std::function<void(const Topic, const Payload&)>* ptr) {
        ESP_LOGI("customDeleter", "Deleting function %p", ptr);
        delete ptr;
    }

    class TestSubscriber : public Subscriber {
    public:
        explicit TestSubscriber(int id) : m_id(id) , _messageVisitor(m_buffer) {}
        const char* getBuffer() const { 
            return m_buffer; 
        }
        uint32_t getCallCount() const { return m_callCount; }
        Topic getTopic() const { return m_topic; }
        void reset() { 
            m_buffer[0] = '\0'; 
            m_topic = 0; 
            m_callCount = 0; 
        }

        void subscriberCallback(const Topic topic, const Payload& message) override {
            m_topic = topic;
            m_callCount++;
            std::visit(_messageVisitor, message);
            ESP_LOGI("subscriberCallback", "id=%d, topic=%d, message='%s'", m_id, topic, m_buffer);
        }

    private:
        static const char* kTag;
        int m_id;
        Topic m_topic = 0;
        char m_buffer[100] = {0};
        uint32_t m_callCount = 0;
        MessageVisitor<100> _messageVisitor;
    };

    const char* TestSubscriber::kTag = "TestSubscriber";

    constexpr const char* TAG = "testPubSub";

    std::atomic<bool> terminateEventLoop(false);

    DEFINE_TEST_CASE(all_payload_types) {
        const char* kTestTag = "AllPayloadTypes";
        PubSub pubsub;
        TestSubscriber subscriber(1);


        // subscribe before event loop started should work
        pubsub.subscribe(&subscriber, 100);

        // subscribe after event loop started should work too
        pubsub.subscribe(&subscriber, 200);

        ESP_LOGI(kTestTag, "Sending int message with subscriber");

        // publish an int message that someone is subscribed to
        pubsub.publish(100, 42);
        
        pubsub.waitForIdle();

        TEST_ASSERT_EQUAL_STRING_MESSAGE("42", subscriber.getBuffer(), "Buffer should contain '42'");
        TEST_ASSERT_EQUAL_MESSAGE(100, subscriber.getTopic(), "Topic should be 100");

        // send a coordinate message to a different topic that is being listened to
        ESP_LOGI(kTestTag, "Sending coordinate message");
        Payload message;
        message.emplace<Coordinate>(100, 200);
        pubsub.publish(200, message);
        pubsub.waitForIdle();
   
        TEST_ASSERT_EQUAL_STRING_MESSAGE("100, 200", subscriber.getBuffer(), "Coordinate correct");
        TEST_ASSERT_EQUAL_MESSAGE(200, subscriber.getTopic(), "Topic should be 200");
        
        // now publish a message that no one is subscribed to
        subscriber.reset();
        ESP_LOGI(kTestTag, "Sending message that nobody is subscribed to");

        pubsub.publish(300, 42);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_STRING_MESSAGE("", subscriber.getBuffer(), "Buffer should still be empty");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber.getTopic(), "Topic should be 0");

        // publish a string message
        Payload string_message = "Hello, World!";
        ESP_LOGI(kTestTag, "Sending string message");
        pubsub.publish(200, string_message);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_STRING_MESSAGE("Hello, World!", subscriber.getBuffer(), "String correct");
        TEST_ASSERT_EQUAL_MESSAGE(200, subscriber.getTopic(), "Topic should be 200 again");
        ESP_LOGI(kTestTag, "Ending all payload types test");
        pubsub.unsubscribeAll();
        terminateEventLoop.store(true);
    }

    DEFINE_TEST_CASE(multiple_subscribers) {
        constexpr const char* kTestTag = "MultipleSubscribers";
        PubSub pubsub;
        TestSubscriber subscriber1(1);
        TestSubscriber subscriber2(2);
        TestSubscriber subscriber3(3);

        // Define the subscriptions
        std::vector<std::pair<Topic, SubscriberHandle>> subscriptions = {
            {1, &subscriber1},
            {2, &subscriber1},
            {2, &subscriber2},
            {2, &subscriber3},
            {3, &subscriber3}
        };

        for (const auto& [topic, callback] : subscriptions) {
            pubsub.subscribe(callback, topic);
        }

        pubsub.publish(2, 42, &subscriber1);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 was not called (subscribed to topic 2 but source)");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber2.getCallCount(), "Subscriber2 was called (subscribed to topic 2)");
        TEST_ASSERT_EQUAL_STRING_MESSAGE("42", subscriber2.getBuffer(), "Subscriber2 content is int 42");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 was called (subscribed to topic 2)");
        TEST_ASSERT_EQUAL_MESSAGE(2, subscriber3.getTopic(), "Subscriber3 topic is 2");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 not called (not subscribed to topic 2)");

        // test that unsubscribe works
        subscriber2.reset();
        subscriber3.reset();
        pubsub.unsubscribe(&subscriber1, 2);
        pubsub.unsubscribe(&subscriber3, 2); 
        ESP_LOGI(kTestTag, "Testing unsubscribe. Sending topic 2 from callback 2 which should not be picked up.");
        pubsub.publish(2, 10, &subscriber2);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 was not called (not subscribed to topic 2)");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (subscribed to topic 2 but it is the source)");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber3.getCallCount(), "Subscriber3 was not called (not subscribed to topic 2)");

        ESP_LOGI(kTestTag, "Testing unsubscribe. Sending topic 3 from callback 2 which subscriber3 should pick up.");

        pubsub.publish(3, 10, &subscriber2);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 was called (still subscribed top topic 3)");
        
        subscriber3.reset();

        ESP_LOGI(kTestTag, "sending topic 1 from callback 2 which subscriber1 should pick up.");
        pubsub.publish(1, 15, &subscriber2);
        pubsub.waitForIdle();

        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getCallCount(), "Subscriber1 was called (subscribed to topic 1)");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getTopic(), "Subscriber1 topic is 1");
        TEST_ASSERT_EQUAL_STRING_MESSAGE("15", subscriber1.getBuffer(), "Subscriber1 content is int 15");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (not subscribed to topic 1)");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber3.getCallCount(), "Subscriber3 was not called (not subscribed to topic 1)");
        subscriber1.reset();

        // subscribing twice still sends one message
    	pubsub.subscribe(&subscriber1, 1);
        ESP_LOGI(kTestTag, "Sending topic 1 from callback 2 to ensure that subscribing twice only results in one execution");

        pubsub.publish(1, 327, &subscriber2);
        pubsub.waitForIdle();

        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getCallCount(), "Subscriber1 was called once, not twice");
        subscriber1.reset();

        // unsubscribing stops update
        pubsub.unsubscribe(&subscriber2, 2);

        pubsub.publish(2, 49);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (not subscribed to 2 anymore)");

        // unsubscribing a topic not subscribed to is handled gracefully
        pubsub.unsubscribe(&subscriber2, 2);

        ESP_LOGI(kTestTag, "Sending topic 2 to ensure that unsubscribing from a non-subscribed topic has no effect");
        pubsub.publish(2, 51);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called after second unsubscribe");

        pubsub.subscribe(&subscriber1, 3);

        ESP_LOGI(kTestTag, "Sending topic 3 from callback2 to check the new subscription gets picked up next to the one that was there");
        pubsub.publish(3, 51, &subscriber2);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getCallCount(), "Subscriber1 was called after new subscribe");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 was called as still subscribed");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (not subscribed to 3)");
        subscriber1.reset();
        subscriber3.reset();
        // unsubscribe a subscriber from all topics
        pubsub.unsubscribe(&subscriber1);

        ESP_LOGI(kTestTag, "Sending topic 1 and 3 from callback2 to check the unsubscribe all works");
        pubsub.publish(3, 69, &subscriber2);
        pubsub.publish(1, 77, &subscriber3);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 not subscribed to 1 or 3");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 still subscribed");
        terminateEventLoop.store(true);
    }
}
