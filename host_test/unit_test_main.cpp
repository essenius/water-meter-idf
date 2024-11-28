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
#include "pub_sub.hpp"
#include "test_pub_sub.hpp"
#include "esp_log.h"

void setUp(void) {
    // Set up code
}

void tearDown(void) {
    // Tear down code
}

constexpr const char* MTAG = "main"; 

extern "C" int main(int argc, char **argv) {
    UNITY_BEGIN();
    ESP_LOGI(MTAG, "Running tests in C++ %ld", __cplusplus);
    RUN_TEST(pubsub::test_all_payload_types);
    RUN_TEST(pubsub::test_multiple_subscribers);
    ESP_LOGI(MTAG, "All tests done");
    int result = UNITY_END();
    deleteAllTasks(); 
    return result;
}