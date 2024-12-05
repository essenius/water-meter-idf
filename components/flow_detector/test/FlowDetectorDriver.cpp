// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.


#include "FlowDetectorDriver.hpp"
#include "PubSub.hpp"

// constructor for ResultAggregatorTest. Uses fields that are used for reporting

namespace flow_detector_test {
    using EllipseMath::EllipseFit;
    using EllipseMath::Coordinate;
    using EllipseMath::CartesianEllipse;

    FlowDetectorDriver::FlowDetectorDriver(std::shared_ptr<pub_sub::PubSub> pubsub, EllipseFit& ellipseFit, const Coordinate& average, 
        const bool pulse, const bool outlier, const bool first)
    : FlowDetector(pubsub, ellipseFit) {
        m_movingAverage = average;
        m_foundPulse = pulse;
        m_foundAnomaly = outlier;
        m_firstCall = first;
        m_wasReset = first;
    }
}
