
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

#include <stdio.h>
#include <string.h>
#include <esp_log.h>

#include "unity.h"

constexpr const char* TAG = "test";

extern "C" void app_main(void)  {
    ESP_LOGI(TAG, "Executing tests in C++ %ld\n", __cplusplus);
    UNITY_BEGIN();
    unity_run_tests_by_tag("[i2c_bus]", false);
    unity_run_tests_by_tag("[magneto_sensor]", false);
    unity_run_tests_by_tag("[pub_sub]", false);

    UNITY_END();

    ESP_LOGI(TAG, "Starting interactive test menu\n");

    // This function will not return, and will be busy waiting for UART input.
    // Make sure that task watchdog is disabled if you use this function.
     unity_run_menu();
}
