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
#include "MagnetoSensorHmc.hpp"
#include "esp_log.h"
#include "MagnetoSensorTest.hpp"

namespace magneto_sensor {
    
    constexpr const char* kHmcTag = "MagnetoSensorHmcTest";

    DEFINE_TEST_CASE(hmc_get_gain) {
        TEST_ASSERT_EQUAL_MESSAGE(1370.0, MagnetoSensorHmc::getGain(HmcRange::Range0_88), "0.88G gain ok");
        TEST_ASSERT_EQUAL_MESSAGE(1090.0, MagnetoSensorHmc::getGain(HmcRange::Range1_3), "1.3G gain ok");
        TEST_ASSERT_EQUAL_MESSAGE(820.0, MagnetoSensorHmc::getGain(HmcRange::Range1_9), "1.9G gain ok");
        TEST_ASSERT_EQUAL_MESSAGE(660.0, MagnetoSensorHmc::getGain(HmcRange::Range2_5), "2.5G gain ok");
        TEST_ASSERT_EQUAL_MESSAGE(440.0, MagnetoSensorHmc::getGain(HmcRange::Range4_0), "4.0G gain ok");
        TEST_ASSERT_EQUAL_MESSAGE(390.0, MagnetoSensorHmc::getGain(HmcRange::Range4_7), "4.7G gain ok");
        TEST_ASSERT_EQUAL_MESSAGE(330.0, MagnetoSensorHmc::getGain(HmcRange::Range5_6), "5.6G gain ok");
        TEST_ASSERT_EQUAL_MESSAGE(230.0, MagnetoSensorHmc::getGain(HmcRange::Range8_1), "8.1G gain ok");
        ESP_LOGI(kHmcTag, "static getGain() test passed. Creating sensor");
        I2cBusMock i2cBus;
        MagnetoSensorHmc sensor(&i2cBus);
        ESP_LOGI(kHmcTag, "Created sensor");
        sensor.configureRange(HmcRange::Range2_5);
        ESP_LOGI(kHmcTag, "Configured range");
        TEST_ASSERT_EQUAL_MESSAGE(660.f, sensor.getGain(), "getGain() returns correct value");
    }

    DEFINE_TEST_CASE(hmc_power_on) {
        ESP_LOGI(kHmcTag, "Creating bus");
        i2c::I2cBus i2cBus; 
        ESP_LOGI(kHmcTag, "Creating sensor");
        MagnetoSensorHmc sensor(&i2cBus);
        
        TEST_ASSERT_TRUE_MESSAGE(sensor.handlePowerOn(), "Sensor Test passed");
    }

    DEFINE_TEST_CASE(hmc_increase_range) {
        I2cBusMock i2cBus; 
        MagnetoSensorHmc sensor(&i2cBus);
        sensor.configureRange(HmcRange::Range0_88);
        TEST_ASSERT_EQUAL_MESSAGE(HmcRange::Range0_88, sensor.getRange(), "Initial range 0.88 OK");
        ESP_LOGI(kHmcTag, "tested range");
        TEST_ASSERT_TRUE_MESSAGE(sensor.increaseRange(), "Increase range from 0.88 OK");
        ESP_LOGI(kHmcTag, "increased range");
        TEST_ASSERT_EQUAL_MESSAGE(HmcRange::Range1_3, sensor.getRange(), "Range 1.3 OK");
        TEST_ASSERT_TRUE_MESSAGE(sensor.increaseRange(), "Increase range from 1.3 OK");
        TEST_ASSERT_EQUAL_MESSAGE(HmcRange::Range1_9, sensor.getRange(), "Range 1.9 OK");
        TEST_ASSERT_TRUE_MESSAGE(sensor.increaseRange(), "Increase range from 1.9 OK");
        TEST_ASSERT_EQUAL_MESSAGE(HmcRange::Range2_5, sensor.getRange(), "Range 2.5 OK");
        TEST_ASSERT_TRUE_MESSAGE(sensor.increaseRange(), "Increase range from 2.5 OK");
        TEST_ASSERT_EQUAL_MESSAGE(HmcRange::Range4_0, sensor.getRange(), "Range 4.0 OK");
        TEST_ASSERT_TRUE_MESSAGE(sensor.increaseRange(), "Increase range from 4.0 OK");
        TEST_ASSERT_EQUAL_MESSAGE(HmcRange::Range4_7, sensor.getRange(), "Range 4.7 OK");
        TEST_ASSERT_TRUE_MESSAGE(sensor.increaseRange(), "Increase range from 4.7 OK");
        TEST_ASSERT_EQUAL_MESSAGE(HmcRange::Range5_6, sensor.getRange(), "Range 5.6 OK");
        TEST_ASSERT_TRUE_MESSAGE(sensor.increaseRange(), "Increase range from 5.6 OK");
        TEST_ASSERT_EQUAL_MESSAGE(HmcRange::Range8_1, sensor.getRange(), "Range 8.1 OK");
        TEST_ASSERT_FALSE_MESSAGE(sensor.increaseRange(), "Increase range from 8.1 not OK");
        TEST_ASSERT_EQUAL_MESSAGE(HmcRange::Range8_1, sensor.getRange(), "Range 8.1 OK");
    }

    DEFINE_TEST_CASE(hmc_test_in_range) {
        SensorData sensorData{200, 400, 400};
        TEST_ASSERT_FALSE_MESSAGE(MagnetoSensorHmc::testInRange(sensorData), "Low X");
        sensorData.x = 600;
        TEST_ASSERT_FALSE_MESSAGE(MagnetoSensorHmc::testInRange(sensorData), "High X");
        sensorData.x = 400;
        TEST_ASSERT_TRUE_MESSAGE(MagnetoSensorHmc::testInRange(sensorData), "Within bounds");
        sensorData.y = 200;
        TEST_ASSERT_FALSE_MESSAGE(MagnetoSensorHmc::testInRange(sensorData), "Low Y");
        sensorData.y = 600;
        TEST_ASSERT_FALSE_MESSAGE(MagnetoSensorHmc::testInRange(sensorData), "High Y");
        sensorData.z = 200;
        TEST_ASSERT_FALSE_MESSAGE(MagnetoSensorHmc::testInRange(sensorData), "Low Z and high Y");
        sensorData.y = 400;
        TEST_ASSERT_FALSE_MESSAGE(MagnetoSensorHmc::testInRange(sensorData), "Low Z");
        sensorData.z = 600;
        TEST_ASSERT_FALSE_MESSAGE(MagnetoSensorHmc::testInRange(sensorData), "High Z");
    }

    DEFINE_TEST_CASE(hmc_configure) {
        I2cBusMock i2cBus;
        MagnetoSensorHmc sensor(&i2cBus);
        sensor.begin();
        constexpr uint8_t BufferBegin[] = {0, 0x78, 1, 0xa0, 2, 0x01, 2, 0x01, 3};
        TEST_ASSERT_EQUAL_MESSAGE(sizeof BufferBegin, i2cBus.writeMismatchIndex(BufferBegin, sizeof BufferBegin), "writes for begin ok");
        TEST_ASSERT_EQUAL_MESSAGE(HmcRange::Range4_7, sensor.getRange(), "Range OK");

        sensor.configureOverSampling(HmcOverSampling::Sampling4); // 0b0100 0000
        sensor.configureRate(HmcRate::Rate30);                    // 0b0001 0100
        sensor.configureRange(HmcRange::Range5_6);                // 0b1100 0000

        // ensure all test variables are reset
        i2cBus.clear();
        sensor.begin();
        constexpr uint8_t BufferReconfigure[] = {0, 0x54, 1, 0xc0, 2, 0x01, 2, 0x01, 3};
        TEST_ASSERT_EQUAL_MESSAGE(sizeof BufferReconfigure, i2cBus.writeMismatchIndex(BufferReconfigure, sizeof BufferReconfigure), "writes for reconfigure ok");
}

    DEFINE_TEST_CASE(hmc_read_sample) {
        I2cBusMock i2cBus;
        MagnetoSensorHmc sensor(&i2cBus);
        sensor.begin();
        i2cBus.clear();
        SensorData sample{};
        constexpr uint8_t expectedOutput[] = {0, 1, 2, 3, 4, 5};
        i2cBus.setOutput(expectedOutput, 6);
        sensor.read(sample);
        // the mock returns values from 0 increasing by 1 for every read
        TEST_ASSERT_EQUAL_MESSAGE(0x001, sample.x, "X ok");
        TEST_ASSERT_EQUAL_MESSAGE(0x0405, sample.y, "Y ok");
        TEST_ASSERT_EQUAL_MESSAGE(0x0203, sample.z, "Z ok");

        // force a lower value than the saturation. This will be 0xeeee, which is -4370, i.e. less than -4096
        constexpr  uint8_t flatLine[] = {0xee, 0xee};
        i2cBus.setOutput(flatLine, 2);        
        TEST_ASSERT_TRUE_MESSAGE(sensor.read(sample), "read succeeded");
        TEST_ASSERT_EQUAL_MESSAGE(SHRT_MIN, sample.x, "X saturated");
        TEST_ASSERT_EQUAL_MESSAGE(SHRT_MIN, sample.y, "Y saturated");
        TEST_ASSERT_EQUAL_MESSAGE(SHRT_MIN, sample.z, "Z saturated");

        i2cBus.clear();
        TEST_ASSERT_FALSE_MESSAGE(sensor.read(sample), "read failed");
    }
}
