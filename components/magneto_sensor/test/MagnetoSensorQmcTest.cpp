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
#include "MagnetoSensorQmc.hpp"
#include "I2cBusMock.hpp"
#include "MagnetoSensorTest.hpp"

namespace magneto_sensor {
    using magneto_sensor::MagnetoSensorQmc;

    DEFINE_TEST_CASE(qmc_get_gain) {
        TEST_ASSERT_EQUAL_MESSAGE(12000.0, MagnetoSensorQmc::getGain(QmcRange::Range2G), "2G gain ok");
        TEST_ASSERT_EQUAL_MESSAGE(3000.0, MagnetoSensorQmc::getGain(QmcRange::Range8G), "8G gain ok");
    }

    DEFINE_TEST_CASE(qmc_get_address) {
        I2cBusMock i2cBusMock;
        MagnetoSensorQmc sensor(&i2cBusMock);
        sensor.begin();
        TEST_ASSERT_EQUAL_MESSAGE(0x0d, i2cBusMock.getAddress(), "Default address OK");

        constexpr uint8_t Address = 0x23;
        sensor.setAddress(Address);
        sensor.begin();
        TEST_ASSERT_EQUAL_MESSAGE(Address, i2cBusMock.getAddress(), "Custom Address OK");
    }

    DEFINE_TEST_CASE(qmc_default_config) {
        I2cBusMock i2cBusMock;
        const MagnetoSensorQmc sensor(&i2cBusMock);
        TEST_ASSERT_EQUAL_MESSAGE(60, sensor.getNoiseRange(), "Noise range is 60");
        TEST_ASSERT_TRUE_MESSAGE(sensor.isReal(), "Sensor is real");
        TEST_ASSERT_EQUAL_MESSAGE(3000.0, sensor.getGain(), "default gain is 3000");
    }

    DEFINE_TEST_CASE(qmc_configure) {
        I2cBusMock i2cBusMock;
        MagnetoSensorQmc sensor(&i2cBusMock);
        sensor.begin();
        constexpr uint8_t BufferBegin[] = {10, 0x80, 11, 0x01, 9, 0x19};
        TEST_ASSERT_EQUAL_MESSAGE(sizeof BufferBegin, i2cBusMock.writeMismatchIndex(BufferBegin, sizeof BufferBegin), "writes for begin ok");

        sensor.configureOverSampling(QmcOverSampling::Sampling128);
        sensor.configureRate(QmcRate::Rate200Hz);
        sensor.configureRange(QmcRange::Range2G);

        // ensure all test variables are reset
        i2cBusMock.clear();
        sensor.begin();
        constexpr uint8_t BufferReconfigure[] = {10, 0x80, 11, 0x01, 9, 0x8d};
        TEST_ASSERT_EQUAL_MESSAGE(sizeof BufferReconfigure, i2cBusMock.writeMismatchIndex(BufferReconfigure, sizeof BufferReconfigure), "Writes for reconfigure ok");
    }


    DEFINE_TEST_CASE(qmc_read_sample) {
        I2cBusMock i2cBusMock;
        MagnetoSensorQmc sensor(&i2cBusMock);
        sensor.begin();
        constexpr uint8_t expectedOutput[] = {0, 1, 2, 3, 4, 5};
        i2cBusMock.setOutput(expectedOutput, 6);
        SensorData sample{};
        sensor.read(sample);
        TEST_ASSERT_EQUAL_MESSAGE(0x0100, sample.x, "X ok");
        TEST_ASSERT_EQUAL_MESSAGE(0x0302, sample.y, "Y ok");
        TEST_ASSERT_EQUAL_MESSAGE(0x0504, sample.z, "Z ok");
    }
}
