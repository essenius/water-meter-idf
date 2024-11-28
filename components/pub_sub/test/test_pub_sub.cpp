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
#include "pub_sub.hpp"
#include <atomic>
#include <thread>
#include <esp_log.h>
#include "test_pub_sub.hpp"


namespace pubsub {
    
    class TestSubscriber {
    public:
        TestSubscriber(int id) : _id(id) , _messageVisitor(_buffer) {}
        const char* getBuffer() const { 
            ESP_LOGI(TAG, "Buffer: %s", _buffer);
            return _buffer; 
        }
        uint32_t getCallCount() const { return _callCount; }
        Topic getTopic() const { return _topic; }
        void reset() { 
            _buffer[0] = '\0'; 
            _topic = 0; 
            _callCount = 0; 
        }

        void subscriberCallback(const Topic topic, const Payload& message) {
            _topic = topic;
            _callCount++;
            std::visit(_messageVisitor, message);
            ESP_LOGI(TAG, "Callback, id=%d, topic=%d, message='%s'", _id, topic, _buffer);
        }

        SubscriberCallback getCallback() {
            return std::make_shared<std::function<void(const Topic, const Payload&)>>(
                [this](const Topic topic, const Payload& message) {
                    this->subscriberCallback(topic, message);
                }
            );
        }

    private:
        static const char* TAG;
        int _id;
        Topic _topic = 0;
        char _buffer[100] = {0};
        uint32_t _callCount = 0;
        MessageVisitor<100> _messageVisitor;
    };

const char* TestSubscriber::TAG = "TestSubscriber";

    constexpr const char* TAG = "testPubSub";

    std::atomic<bool> terminateEventLoop(false);

#ifdef ESP_PLATFORM
TEST_CASE("All payload types", "[pub_sub]") {
#else
    void test_all_payload_types() {
#endif
        const char* TAG = "testPubSub-AllPayloadTypes";
        ESP_LOGI(TAG, "*** Starting all payload types test ***\n");
        PubSub pubsub;
        TestSubscriber subscriber(1);

        SubscriberCallback callback = subscriber.getCallback();

        // subscribe before event loop started should work
        pubsub.subscribe(100, callback);

        // subscribe after event loop started should work too
        pubsub.subscribe(200, callback);

        ESP_LOGI(TAG, "Sending int message with subscriber");

        // publish an int message that someone is subscribed to
        pubsub.publish(100, 42);
        
        pubsub.waitForIdle();

        TEST_ASSERT_EQUAL_STRING_MESSAGE("int: 42", subscriber.getBuffer(), "Buffer should contain 'int: 42'");
        TEST_ASSERT_EQUAL_MESSAGE(100, subscriber.getTopic(), "Topic should be 100");

        // send a coordinate message to a different topic that is being listened to
        ESP_LOGI(TAG, "Sending coordinate message");
        Payload message;
        message.emplace<Coordinate>(100, 200);
        pubsub.publish(200, message);
        pubsub.waitForIdle();
   
        TEST_ASSERT_EQUAL_STRING_MESSAGE("coordinate: x=100, y=200", subscriber.getBuffer(), "Coordinate correct");
        TEST_ASSERT_EQUAL_MESSAGE(200, subscriber.getTopic(), "Topic should be 200");
        
        // now publish a message that no one is subscribed to
        subscriber.reset();
        ESP_LOGI(TAG, "Sending message that nobody is subscribed to");

        pubsub.publish(300, 42);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_STRING_MESSAGE("", subscriber.getBuffer(), "Buffer should still be empty");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber.getTopic(), "Topic should be 0");

        // publish a string message
        Payload string_message = "Hello, World!";
        ESP_LOGI(TAG, "Sending string message");
        pubsub.publish(200, string_message);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_STRING_MESSAGE("string: Hello, World!", subscriber.getBuffer(), "String correct");
        TEST_ASSERT_EQUAL_MESSAGE(200, subscriber.getTopic(), "Topic should be 200 again");
        ESP_LOGI(TAG, "Ending all payload types test");
        //pubsub.unsubscribeAll();
        terminateEventLoop.store(true);
    }

#ifdef ESP_PLATFORM
TEST_CASE("Multiple subscribers", "[pub_sub]") {
#else
void test_multiple_subscribers() {
#endif
        constexpr const char* TAG = "testPubSub-MultipleSubscribers";
        ESP_LOGI(TAG, "\n*** Starting multiple subscribers test ***\n");
        PubSub pubsub;
        TestSubscriber subscriber1(1);
        TestSubscriber subscriber2(2);
        TestSubscriber subscriber3(3);

        SubscriberCallback callback1 = subscriber1.getCallback();
        SubscriberCallback callback2 = subscriber2.getCallback();
        SubscriberCallback callback3 = subscriber3.getCallback();

        // Define the subscriptions
        std::vector<std::pair<Topic, SubscriberCallback>> subscriptions = {
            {1, callback1},
            {2, callback1},
            {2, callback2},
            {2, callback3},
            {3, callback3}
        };

        for (const auto& [topic, callback] : subscriptions) {
            pubsub.subscribe(topic, callback);
        }

        ESP_LOGI(TAG, "Sending message to topic 2 from callback1");
        pubsub.publish(2, 42, callback1);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 was not called (subscribed to topic 2 but source)");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber2.getCallCount(), "Subscriber2 was called (subscribed to topic 2)");
        TEST_ASSERT_EQUAL_STRING_MESSAGE("int: 42", subscriber2.getBuffer(), "Subscriber2 content is int 42");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 was called (subscribed to topic 2)");
        TEST_ASSERT_EQUAL_MESSAGE(2, subscriber3.getTopic(), "Subscriber3 topic is 2");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 not called (not subscribed to topic 2)");

        // test that unsubscribe works
        subscriber2.reset();
        subscriber3.reset();
        pubsub.unsubscribe(2, callback1);
        pubsub.unsubscribe(2, callback3); 
        ESP_LOGI(TAG, "Testing unsubscribe. Sending topic 2 from callback 2 which should not be picked up");
        pubsub.publish(2, 10, callback2);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 was not called (not subscribed to topic 2)");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (subscribed to topic 2 but it is the source)");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber3.getCallCount(), "Subscriber3 was not called (not subscribed to topic 2)");

        ESP_LOGI(TAG, "Testing unsubscribe. Sending topic 3 from callback 2 which subscriber3 should pick up");

        pubsub.publish(3, 10, callback2);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 was called (still subscribed top topic 3)");
        
        subscriber3.reset();

        ESP_LOGI(TAG, "sending topic 1 from callback 2 which subscriber1 should pick up");
        pubsub.publish(1, 15, callback2);
        pubsub.waitForIdle();

        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getCallCount(), "Subscriber1 was called (subscribed to topic 1)");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getTopic(), "Subscriber1 topic is 1");
        TEST_ASSERT_EQUAL_STRING_MESSAGE("int: 15", subscriber1.getBuffer(), "Subscriber1 content is int 15");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (not subscribed to topic 1)");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber3.getCallCount(), "Subscriber3 was not called (not subscribed to topic 1)");

        subscriber1.reset();

        // subscribing twice still sends one message
        pubsub.dump_subscribers("before duplicate subscription on topic 1");

    	pubsub.subscribe(1, callback1);
        ESP_LOGI(TAG, "Sending topic 1 from callback 2 to ensure that subscribing twice only results in one execution");

        pubsub.dump_subscribers("after duplicate subscription on topic 1");

        pubsub.publish(1, 327, callback2);
        pubsub.dump_subscribers("after publish 327");
        ESP_LOGI(TAG, "waiting for idle after sending 327"); 
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getCallCount(), "Subscriber1 was called once, not twice");
        ESP_LOGI(TAG, "passed idle and test"); 
        subscriber1.reset();

        // unsubscribing stops update
        pubsub.unsubscribe(2, callback2);

        ESP_LOGI(TAG, "passed unsubscribe"); 
        pubsub.publish(2, 49);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (not subscribed to 2 anymore)");

        // unsubscribing a topic not subscribed to is handled gracefully
        pubsub.unsubscribe(2, callback2);
        ESP_LOGI(TAG, "Sending topic 2 to ensure that unsubscribing from a non-subscribed topic has no effect");
        pubsub.publish(2, 51);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called after second unsubscribe");

        pubsub.subscribe(3, callback1);

        ESP_LOGI(TAG, "Sending topic 3 from callback2 to check the new subscription gets picked up next to the one that was there");
        pubsub.publish(3, 51, callback2);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getCallCount(), "Subscriber1 was called after new subscribe");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 was called as still subscribed");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (not subscribed to 3)");
        subscriber1.reset();
        subscriber3.reset();
        // unsubscribe a subscriber from all topics
        pubsub.unsubscribe(callback1);
        ESP_LOGI(TAG, "Sending topic 1 and 3 from callback2 to check the unsubscribe all works");
        pubsub.publish(3, 69, callback2);
        pubsub.publish(1, 77, callback3);
        pubsub.waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 not subscribed to 1 or 3");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 still subscribed");
        terminateEventLoop.store(true);
    }
}
