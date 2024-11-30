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

#include "unity.h"
#include "MagnetoSensorNull.hpp"
#include "TestMagnetoSensor.hpp"

#ifdef ESP_PLATFORM
#define DEFINE_TEST_CASE(test_name) \
    TEST_CASE(#test_name, "[magneto_sensor]") 
#else
#define DEFINE_TEST_CASE(test_name) \
    void test_##test_name()
#endif

namespace magneto_sensor {
    using magneto_sensor::MagnetoSensorNull;
    using magneto_sensor::SensorData;

    DEFINE_TEST_CASE(null_sensor_script) {
        MagnetoSensorNull nullSensor;
        TEST_ASSERT_FALSE_MESSAGE(nullSensor.begin(), "begin() returns false");
        TEST_ASSERT_TRUE_MESSAGE(nullSensor.isOn(), "IsOn() returns true");
        nullSensor.waitForPowerOff(); // doesn't wait
        TEST_ASSERT_TRUE_MESSAGE(nullSensor.handlePowerOn(), "handlePowerOn returns true");
        TEST_ASSERT_EQUAL_MESSAGE(0.0, nullSensor.getGain(), "Gain is 0");
        TEST_ASSERT_EQUAL_MESSAGE(0, nullSensor.getNoiseRange(), "Noise range is 0");
        SensorData sample{1, 1, 0};
        TEST_ASSERT_FALSE_MESSAGE(nullSensor.isReal(), "nullSensor is not a real sensor");
        TEST_ASSERT_FALSE_MESSAGE(nullSensor.read(sample), "Read returns false");
        TEST_ASSERT_EQUAL_MESSAGE(0, sample.y, "y was reset");
        // validate that it doesn't break
        nullSensor.softReset();
    }
}
