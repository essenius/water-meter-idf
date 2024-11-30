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

#include "unity.h"
#include "TestMagnetoSensor.hpp"

namespace magneto_sensor {
    DEFINE_TEST_CASE(sensor_data_saturated) {
        SensorData data;
        data.reset();
        TEST_ASSERT_FALSE_MESSAGE(data.isSaturated(), "Not saturated");
        data.x = SHRT_MIN;
        TEST_ASSERT_TRUE_MESSAGE(data.isSaturated(), "Saturated on x");
        data.reset();
        data.y = SHRT_MIN;
        TEST_ASSERT_TRUE_MESSAGE(data.isSaturated(), "Saturated on y");
        data.reset();
        data.z = SHRT_MIN;
        TEST_ASSERT_TRUE_MESSAGE(data.isSaturated(), "Saturated on z");
    }
}