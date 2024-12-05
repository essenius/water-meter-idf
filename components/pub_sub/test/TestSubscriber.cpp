
#include "TestSubscriber.hpp"
#include "PubSub.hpp"
#include "esp_log.h"
#include <variant>

namespace pub_sub_test {
        
    void TestSubscriber::reset() { 
        Subscriber::reset();
        m_buffer[0] = '\0'; 
        m_callCount = 0; 
    }

    void TestSubscriber::subscriberCallback(const Topic topic, const Payload& payload) {
        Subscriber::subscriberCallback(topic, payload);
        m_callCount++;
        std::visit(m_messageVisitor, payload);
        ESP_LOGI("subscriberCallback", "id=%d, topic=%d, message='%s'", m_id, static_cast<uint8_t>(topic), m_buffer);
    }

}