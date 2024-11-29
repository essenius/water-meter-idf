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

#include "I2cBusMock.hpp"
#include "MagnetoSensor.hpp"

#ifdef ESP_PLATFORM
#define DEFINE_TEST_CASE(test_name) \
    TEST_CASE(#test_name, "[magneto_sensor]") 
#else
#define DEFINE_TEST_CASE(test_name) \
    void test_##test_name()
#endif

namespace magneto_sensor {
    void test_sensor_is_on();
}