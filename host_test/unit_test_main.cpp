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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "PubSub.hpp"
#include "TestPubSub.hpp"
#include "TestI2cBus.hpp"
#include "TestMagnetoSensor.hpp"
#include "esp_log.h"
#include "FlowDetector.hpp"
#include "TestFlowDetector.hpp"	

#include "driver/i2c_master.h"

void setUp(void) {
    // use values that would pass the test in hmc (i.e. between 243 and 575)
    constexpr uint8_t expectedOutput[] = {0x00, 0xf4, 0x01, 0x80, 0x02, 0x3e};
    i2c_master_test::setOutput(expectedOutput, sizeof expectedOutput);
}

void tearDown(void) {
    // Tear down code
}

constexpr const char* MTAG = "main"; 

extern "C" int main(int argc, char **argv) {
    UNITY_BEGIN();
    ESP_LOGI(MTAG, "Running tests in C++ %ld", __cplusplus);
    
    i2c::run_tests();
    magneto_sensor::run_tests();
    pub_sub_test::run_tests();
    flow_detector_test::run_tests();

    ESP_LOGI(MTAG, "All tests done");
    int result = UNITY_END();
    deleteAllTasks(); 
    return result;
}