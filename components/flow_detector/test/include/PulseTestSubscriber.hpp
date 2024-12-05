// Copyright 2022-2024 Rik Essenius
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
#include <fstream>
#include <sstream>

#include "TestSubscriber.hpp"
#include "PubSub.hpp"

namespace flow_detector_test {
    using pub_sub::PubSub;
    using pub_sub::Topic;
    using pub_sub::Payload;
    using pub_sub::IntCoordinate;
    using pub_sub::Subscriber;

    class PulseTestSubscriber : public Subscriber {
    public:
        PulseTestSubscriber(PubSub& pubsub, const char* fileName = nullptr);
        unsigned int anomalies() const { return m_excludeCount; }
        unsigned int drifts() const { return m_driftCount; }
        unsigned int noFits() const { return m_noFitCount; }
        const char* pulseHistory() const { return m_buffer; }
        unsigned int pulses(const bool stage) const { return m_pulseCount[stage]; }
        void subscriberCallback(const Topic topic, const Payload& payload) override;
 
        void writeAttributes();
        void close();

    private:
        char m_buffer[4096] = {};
        IntCoordinate m_currentSample = {};
        unsigned int m_driftCount = 0;
        unsigned int m_excludeCount = 0;
        unsigned int m_pulseCount[2] = {};
        unsigned int m_noFitCount = 0;
        unsigned int m_sampleNumber = -1;
        bool m_writeToFile = false;
        std::ofstream m_file;
        std::stringstream m_line;
        bool m_anomaly = false;
        bool m_noFit = false;
        bool m_drift = false;
        bool m_pulse = false;
    };
}
