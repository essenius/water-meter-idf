#pragma once

#include "PubSub.hpp"

namespace pub_sub_test {
    using pub_sub::Subscriber;
    using pub_sub::Topic;
    using pub_sub::Payload;
    using pub_sub::MessageVisitor;

    class TestSubscriber : public Subscriber {
    public:
        explicit TestSubscriber(int id) : m_id(id) , m_messageVisitor(m_buffer) {}
        const char* getBuffer() const { 
            return m_buffer; 
        }
        uint32_t getCallCount() const { return m_callCount; }

        void reset() override;

        void subscriberCallback(const Topic topic, const Payload& payload) override;

    private:
        static const char* kTag;
        int m_id;
        char m_buffer[100] = {0};
        uint32_t m_callCount = 0;
        MessageVisitor<100> m_messageVisitor;
    };
}
