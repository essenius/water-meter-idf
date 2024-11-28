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

// We ignore a few static analysis findings for code clarity. The issues are related to:
// * conversions that might in theory go wrong, but the sensor won't return too high values.
// * using enum values to do bitwise manipulations

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "MagnetoSensorHmc.hpp"
#include <cstddef>
#include <utility>

namespace magneto_sensor {

    constexpr const char* kTag = "MagnetoSensorHmc";

    MagnetoSensorHmc::MagnetoSensorHmc(i2c::I2cBus* i2cBus, const uint8_t address) : MagnetoSensor(i2cBus, address)  { 
        m_address = address;
    }

    void MagnetoSensorHmc::configure(const uint8_t range, const uint8_t bias) const {
        ESP_LOGI(kTag, "Configuring HMC5883L sensor, oversampling 0x%02x, rate 0x%02x, bias 0x%02x, range 0x%02x", m_overSampling, m_rate, bias, range);
        m_i2cBus->writeRegister(HmcRegister::ControlA, m_overSampling + m_rate + bias);
        m_i2cBus->writeRegister(HmcRegister::ControlB, range);
    }

    void MagnetoSensorHmc::configureRange(const uint8_t range) {
        m_range = range;
    }

    void MagnetoSensorHmc::configureOverSampling(const uint8_t overSampling) {
        ESP_LOGI(kTag, "Oversampling = 0x%02x", overSampling);
        m_overSampling = overSampling;
    }

    void MagnetoSensorHmc::configureRate(const uint8_t rate) {
        m_rate = rate;
    }

    double MagnetoSensorHmc::getGain() const {
        return getGain(m_range);
    }

    uint8_t MagnetoSensorHmc::getRange() const {
        return m_range;
    }

    int MagnetoSensorHmc::getNoiseRange() const {
        switch (m_range) {
            case HmcRange::Range0_88: return 8;
            case HmcRange::Range1_3:
            case HmcRange::Range1_9: return 5;
            case HmcRange::Range2_5:
            case HmcRange::Range4_0: return 4;
            case HmcRange::Range4_7: return 3; // was 4
            case HmcRange::Range5_6:
            case HmcRange::Range8_1: return 2;
            default: return 0; // should not happen
        }
    }

    double MagnetoSensorHmc::getGain(const uint8_t range) {
        switch (range) {
            case HmcRange::Range0_88: return 1370.0;
            case HmcRange::Range1_3: return 1090.0;
            case HmcRange::Range1_9: return 820.0;
            case HmcRange::Range2_5: return 660.0;
            case HmcRange::Range4_0: return 440.0;
            case HmcRange::Range4_7: return 390.0;
            case HmcRange::Range5_6: return 330.0;
            case HmcRange::Range8_1: return 230.0;
            default: return 0.0; // should not happen
        }
    }

    void MagnetoSensorHmc::getTestMeasurement(SensorData& reading) {
        startMeasurement();
        vTaskDelay(pdMS_TO_TICKS(5));
        read(reading);
    }

    int16_t MagnetoSensorHmc::getWord(const uint8_t* input) const {

        // order is MSB, LSB
        ESP_LOGI(kTag, "Input[0] = 0x%02x, Input[1] = 0x%02x", input[0], input[1]);
        int16_t result = input[0] * 0x100 + input[1];

        // harmonize saturation values across sensors
        if (result <= Saturated) {
            result = SHRT_MIN;
        }
        ESP_LOGI(kTag, "Result = 0x%04x", result);
        return result;
    }

    bool MagnetoSensorHmc::read(SensorData& sample) {
        startMeasurement();

        //Read data from each axis, 2 registers per axis
        // order: x MSB, x LSB, z MSB, z LSB, y MSB, y LSB
        constexpr int BytesToRead = 6;
        uint8_t output[BytesToRead];
        if (m_i2cBus->writeByteAndReadData(HmcRegister::Data, output, BytesToRead) != ESP_OK) return false;
        for (int i = 0; i < BytesToRead; i++) {
            ESP_LOGI(kTag, "Output[%d] = 0x%02x", i, output[i]);
        }
        sample.x = getWord(output);
        sample.z = getWord(output + 2);
        sample.y = getWord(output + 4);
        ESP_LOGI(kTag, "Read sample x: 0x%04x, y: 0x%04x, z: 0x%04x", sample.x, sample.y, sample.z);
        return true;
    }

    void MagnetoSensorHmc::softReset() {
        configure(m_range, HmcBias::None);
        SensorData sample{};
        getTestMeasurement(sample);
    }

    void MagnetoSensorHmc::startMeasurement() const {
        m_i2cBus->writeRegister(HmcRegister::Mode, HmcMode::Single);
    }

    bool MagnetoSensorHmc::testInRange(const SensorData& sample) {
        constexpr short LowThreshold = 243;
        constexpr short HighThreshold = 575;

        return
            sample.x >= LowThreshold && sample.x <= HighThreshold &&
            sample.y >= LowThreshold && sample.y <= HighThreshold &&
            sample.z >= LowThreshold && sample.z <= HighThreshold;
    }

    bool MagnetoSensorHmc::test() {
        SensorData sample{};

        configure(HmcRange::Range4_7, HmcBias::Positive);

        // read old value (still with old settings) 
        getTestMeasurement(sample);
        // the first one with new settings may still be a bit off
        getTestMeasurement(sample);

        // now do the test
        getTestMeasurement(sample);
        const bool passed = testInRange(sample);

        // end self test mode
        configure(m_range, HmcBias::None);
        // skip the final measurement with the old gain
        getTestMeasurement(sample);

        return passed;
    }

    bool MagnetoSensorHmc::handlePowerOn() {
        return test();
    }

    bool MagnetoSensorHmc::increaseRange() {
        if (m_range == HmcRange::Range8_1) return false;
        m_range += 32;
        softReset();
        return true;
    }
}
