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
#include "TestSubscriber.hpp"


namespace pub_sub_test {
    using pub_sub::PubSub;
    using pub_sub::Topic;
    using pub_sub::Payload;
    using pub_sub::IntCoordinate;
    using pub_sub::Subscriber;
    using pub_sub::SubscriberHandle;
    using pub_sub::MessageVisitor;
    using pub_sub_test::TestSubscriber;
    
    //std::atomic<bool> terminateEventLoop(false);

    DEFINE_TEST_CASE(all_payload_types) {
        {
        const char* kTestTag = "AllPayloadTypes";
        auto pubsub = PubSub::create();
        TestSubscriber subscriber(1);

        // subscribe before event loop started should work
        pubsub->subscribe(&subscriber, Topic::Pulse);

        // subscribe after event loop started should work too
        pubsub->subscribe(&subscriber, Topic::Sample);

        ESP_LOGI(kTestTag, "Sending int message with subscriber");

        // publish an int message that someone is subscribed to
        pubsub->publish(Topic::Pulse, 42);
        
        pubsub->waitForIdle();

        TEST_ASSERT_EQUAL_STRING_MESSAGE("42", subscriber.getBuffer(), "Buffer should contain '42'");
        TEST_ASSERT_EQUAL_MESSAGE(Topic::Pulse, subscriber.getTopic(), "Topic should be Topic::Pulse");

        // send a coordinate message to a different topic that is being listened to
        ESP_LOGI(kTestTag, "Sending coordinate message");
        Payload message;
        message.emplace<IntCoordinate>(100, 200);
        pubsub->publish(Topic::Sample, message);
        pubsub->waitForIdle();
   
        TEST_ASSERT_EQUAL_STRING_MESSAGE("100, 200", subscriber.getBuffer(), "Coordinate correct");
        TEST_ASSERT_EQUAL_MESSAGE(Topic::Sample, subscriber.getTopic(), "Topic should be Topic::Sample");
        
        // now publish a message that no one is subscribed to
        subscriber.reset();
        ESP_LOGI(kTestTag, "Sending message that nobody is subscribed to");

        pubsub->publish(Topic::NoFit, 42);
        pubsub->waitForIdle();
        TEST_ASSERT_EQUAL_STRING_MESSAGE("", subscriber.getBuffer(), "Buffer should still be empty");
        TEST_ASSERT_EQUAL_MESSAGE(Topic::None, subscriber.getTopic(), "Topic should be Topic::None");

        // publish a string message
        Payload string_message = "Hello, World!";
        ESP_LOGI(kTestTag, "Sending string message");
        pubsub->publish(Topic::Sample, string_message);
        pubsub->waitForIdle();
        TEST_ASSERT_EQUAL_STRING_MESSAGE("Hello, World!", subscriber.getBuffer(), "String correct");
        TEST_ASSERT_EQUAL_MESSAGE(Topic::Sample, subscriber.getTopic(), "Topic should be Topic::Sample again");
        ESP_LOGI(kTestTag, "Ending all payload types test");
        pubsub->unsubscribeAll();
        pubsub->end();
        }
    }

    DEFINE_TEST_CASE(multiple_subscribers) {
        {
        constexpr const char* kTestTag = "MultipleSubscribers";
        auto pubsub = PubSub::create();
        TestSubscriber subscriber1(1);
        TestSubscriber subscriber2(2);
        TestSubscriber subscriber3(3);

        // Define the subscriptions
        std::vector<std::pair<Topic, SubscriberHandle>> subscriptions = {
            {Topic::Anomaly, &subscriber1},
            {Topic::Sample, &subscriber1},
            {Topic::Sample, &subscriber2},
            {Topic::Sample, &subscriber3},
            {Topic::Pulse, &subscriber3}
        };

        for (const auto& [topic, callback] : subscriptions) {
            pubsub->subscribe(callback, topic);
        }

        pubsub->publish(Topic::Sample, 42, &subscriber1);
        pubsub->waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 was not called (subscribed to topic Sample but source)");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber2.getCallCount(), "Subscriber2 was called (subscribed to topic Sample)");
        TEST_ASSERT_EQUAL_STRING_MESSAGE("42", subscriber2.getBuffer(), "Subscriber2 content is int 42");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 was called (subscribed to topic Sample)");
        TEST_ASSERT_EQUAL_MESSAGE(Topic::Sample, subscriber3.getTopic(), "Subscriber3 topic is Sample");

        // test that unsubscribe works
        subscriber2.reset();
        subscriber3.reset();
        pubsub->unsubscribe(&subscriber1, Topic::Sample);
        pubsub->unsubscribe(&subscriber3, Topic::Sample); 
        ESP_LOGI(kTestTag, "Testing unsubscribe. Sending topic Sample from subscriber2 which should not be picked up.");
        pubsub->publish(Topic::Sample, 10, &subscriber2);
        pubsub->waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 was not called (not subscribed to topic Sample)");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (subscribed to topic Sample but it is the source)");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber3.getCallCount(), "Subscriber3 was not called (not subscribed to topic Sample)");

        ESP_LOGI(kTestTag, "Testing unsubscribe. Sending topic Pulse from callback 2 which subscriber3 should pick up.");

        pubsub->publish(Topic::Pulse, 10, &subscriber2);
        pubsub->waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 was called (still subscribed top topic Pulse)");
        
        subscriber3.reset();

        ESP_LOGI(kTestTag, "sending topic Anomaly from subscriber2 which subscriber1 should pick up.");
        pubsub->publish(Topic::Anomaly, 15, &subscriber2);
        pubsub->waitForIdle();

        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getCallCount(), "Subscriber1 was called (subscribed to topic Anomaly)");
        TEST_ASSERT_EQUAL_MESSAGE(Topic::Anomaly, subscriber1.getTopic(), "Subscriber1 topic is Topic::Anomaly");
        TEST_ASSERT_EQUAL_STRING_MESSAGE("15", subscriber1.getBuffer(), "Subscriber1 content is int 15");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (not subscribed to topic Anomaly)");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber3.getCallCount(), "Subscriber3 was not called (not subscribed to topic Anomaly)");
        subscriber1.reset();

        // subscribing twice still sends one message
    	pubsub->subscribe(&subscriber1, Topic::Anomaly);
        ESP_LOGI(kTestTag, "Sending topic Anomaly from subscriber2 to ensure that subscribing twice only results in one execution");

        pubsub->publish(Topic::Anomaly, 327, &subscriber2);
        pubsub->waitForIdle();

        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getCallCount(), "Subscriber1 was called once, not twice");
        subscriber1.reset();

        // unsubscribing stops update
        pubsub->unsubscribe(&subscriber2, Topic::Sample);

        pubsub->publish(Topic::Sample, 49);
        pubsub->waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (not subscribed to Sample anymore)");

        // unsubscribing a topic not subscribed to is handled gracefully
        pubsub->unsubscribe(&subscriber2, Topic::Sample);

        ESP_LOGI(kTestTag, "Sending topic Sample to ensure that unsubscribing from a non-subscribed topic has no effect");
        pubsub->publish(Topic::Sample, 51);
        pubsub->waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called after second unsubscribe");

        pubsub->subscribe(&subscriber1, Topic::Pulse);

        ESP_LOGI(kTestTag, "Sending topic Pulse from subscriber2 to check the new subscription gets picked up next to the one that was there");
        pubsub->publish(Topic::Pulse, 51, &subscriber2);
        pubsub->waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber1.getCallCount(), "Subscriber1 was called after new subscribe");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 was called as still subscribed");
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber2.getCallCount(), "Subscriber2 was not called (not subscribed to Pulse)");
        subscriber1.reset();
        subscriber3.reset();
        // unsubscribe a subscriber from all topics
        pubsub->unsubscribe(&subscriber1);

        ESP_LOGI(kTestTag, "Sending topic 1 and 3 from subscriber2 to check the unsubscribe all works");
        pubsub->publish(Topic::Pulse, 69, &subscriber2);
        pubsub->publish(Topic::Anomaly, 77, &subscriber3);
        pubsub->waitForIdle();
        TEST_ASSERT_EQUAL_MESSAGE(0, subscriber1.getCallCount(), "Subscriber1 not subscribed to Anomaly or Pulse");
        TEST_ASSERT_EQUAL_MESSAGE(1, subscriber3.getCallCount(), "Subscriber3 still subscribed");
        pubsub->end();
        }
    }
}
