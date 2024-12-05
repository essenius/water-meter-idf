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


#pragma once

#include "FlowDetector.hpp"

namespace flow_detector_test {
    using flow_detector::FlowDetector;
    using EllipseMath::EllipseFit;
    using EllipseMath::Coordinate;
    using pub_sub::PubSub;

    class FlowDetectorDriver final : public FlowDetector {
    public:
        using FlowDetector::addSample;
        using FlowDetector::detectPulse;
        using FlowDetector::processMovingAverageSample;
        using FlowDetector::m_movingAverageArray;
        using FlowDetector::m_movingAverage;
        using FlowDetector::m_justStarted;
        using FlowDetector::m_foundPulse;

        explicit FlowDetectorDriver(PubSub& pubsub, EllipseFit& ellipseFit) : FlowDetector(pubsub, ellipseFit) {}

        FlowDetectorDriver(PubSub& pubsub, EllipseFit& ellipseFit, const Coordinate& average, 
                           bool pulse = false, bool outlier = false, bool first = false);

    };
}
