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

    namespace magneto_sensor {
    void test_sensor_is_on();

    void test_sensor_data_saturated();

    void test_null_sensor_script();

    void test_qmc_get_gain();
    void test_qmc_get_address();
    void test_qmc_default_config();
    void test_qmc_configure();
    void test_qmc_read_sample();

    void test_hmc_get_gain();
    void test_hmc_power_on();
    void test_hmc_increase_range();
    void test_hmc_test_in_range();
    void test_hmc_configure();
    void test_hmc_read_sample();

    inline void run_tests() {
        RUN_TEST(test_sensor_is_on);

        RUN_TEST(test_sensor_data_saturated);

        RUN_TEST(test_null_sensor_script);

        RUN_TEST(test_qmc_get_gain);
        RUN_TEST(test_qmc_get_address);
        RUN_TEST(test_qmc_default_config);
        RUN_TEST(test_qmc_configure);
        RUN_TEST(test_qmc_read_sample);

        RUN_TEST(test_hmc_get_gain);
        RUN_TEST(test_hmc_power_on);
        RUN_TEST(test_hmc_increase_range);
        RUN_TEST(test_hmc_test_in_range);
        RUN_TEST(test_hmc_configure);
        RUN_TEST(test_hmc_read_sample);
    }
}
#endif

