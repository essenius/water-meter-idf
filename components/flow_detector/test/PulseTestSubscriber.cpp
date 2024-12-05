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

#include "PulseTestSubscriber.hpp"

#include <fstream>
#include <iostream>
#include <SafeCString.h>

namespace flow_detector_test {
    using pub_sub::PubSub;
    using pub_sub::Topic;
    using pub_sub::Payload;
    using pub_sub::IntCoordinate;
    using pub_sub_test::TestSubscriber;

    PulseTestSubscriber::PulseTestSubscriber(std::shared_ptr<pub_sub::PubSub> pubsub, const char* fileName) {
#ifdef ESP_PLATFORM
        m_writeToFile = false;
#else
        m_writeToFile = fileName != nullptr;
#endif
        if (m_writeToFile) {
            m_file.open(fileName);            
            std::cout << "Writing to " << fileName << "\n";
            m_file << "SampleNo,X,Y,Pulse,Anomaly,NoFit,Drift\n";
        }

        pubsub->subscribe(this, Topic::Anomaly);
        pubsub->subscribe(this, Topic::Drifted);
        pubsub->subscribe(this, Topic::NoFit);
        pubsub->subscribe(this, Topic::Pulse);
        pubsub->subscribe(this, Topic::Sample);
    }

    void PulseTestSubscriber::subscriberCallback(const Topic topic, const Payload& payload) {
        Subscriber::subscriberCallback(topic, payload);
        if (topic == Topic::Pulse) {
            int value = std::get<int>(payload);
            m_pulseCount[value]++;
            char numberBuffer[32];
            SafeCString::sprintf(numberBuffer, "[%d:%d,%d]\n", m_sampleNumber, m_currentSample.x, m_currentSample.y);
            SafeCString::strcat(m_buffer, numberBuffer);
            m_pulse = true;
        }
        else if (topic == Topic::Anomaly) {
            m_excludeCount++;
            m_anomaly = true;
        }
        else if (topic == Topic::NoFit) {
            m_noFitCount++;
            m_noFit = true;
        }
        else if (topic == Topic::Drifted) {
            m_driftCount++;
            m_drift = true;
        } else if (topic == Topic::Sample) {
            auto value = std::get<IntCoordinate>(payload);
            m_sampleNumber++;
            m_currentSample = value;
            if (!m_writeToFile) return;
            // write previous sample attributes
            if (m_sampleNumber > 0) writeAttributes();
            auto coordinate = std::get<IntCoordinate>(payload);
            m_line << m_sampleNumber << "," << coordinate.x << "," << coordinate.y;
            m_file << m_line.str();
            m_line.str("");
            m_line.clear();
            m_pulse = false;
            m_anomaly = false;
            m_drift = false;
            m_noFit = false;
        }
    }

    void PulseTestSubscriber::writeAttributes() {
        m_line << "," << m_pulse << "," << m_anomaly << "," << m_noFit << "," << m_drift << "\n";
    }

    void PulseTestSubscriber::close() {
        if (m_writeToFile) {
            writeAttributes();
            m_file << m_line.str();
            m_file.close();
            std::cout << "Done writing" << "\n";
        }
    }
}