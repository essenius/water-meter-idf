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

#include "MagnetoSensorQmc.hpp"

namespace magneto_sensor {
    constexpr int SoftReset = 0x80;

    MagnetoSensorQmc::MagnetoSensorQmc(i2c::I2cBus* i2cBus, const uint8_t address) : MagnetoSensor(i2cBus, address) {
        m_address = address;
    }

    bool MagnetoSensorQmc::configure() const {
        m_i2cBus->writeRegister(QmcRegister::SetReset, 0x01);
        m_i2cBus->writeRegister(QmcRegister::Control1, QmcMode::Continuous + m_rate + m_range + m_overSampling);
        return true;
    }

    void MagnetoSensorQmc::configureOverSampling(const uint8_t overSampling) {
        m_overSampling = overSampling;
    }

    void MagnetoSensorQmc::configureRange(const uint8_t range) {
        m_range = range;
    }

    void MagnetoSensorQmc::configureRate(const uint8_t rate) {
        m_rate = rate;
    }

    // if we ever need DataReady, use (getRegister(QmcStatus) & 0x01) != 0;

    double MagnetoSensorQmc::getGain() const {
        return getGain(m_range);
    }

    double MagnetoSensorQmc::getGain(const uint8_t range) {
        if (range == QmcRange::Range8G) return 3000.0;
        return 12000.0;
    }

    uint8_t MagnetoSensorQmc::getRange() const {
        return m_range;
    }

    int16_t MagnetoSensorQmc::getWord(const uint8_t* input) const {

        // order is LSB, MSB
        int16_t result = input[0] + 0x100 * input[1];

        // if we got a positive saturation, shift it to SHRT_MIN as SHRT_MAX means an error
        if (result == SHRT_MAX) result = SHRT_MIN;
        return result;
    }

    bool MagnetoSensorQmc::read(SensorData& sample) {

        //Read data from each axis, 2 registers per axis
        // order: x MSB, x LSB, z MSB, z LSB, y MSB, y LSB
        constexpr int BytesToRead = 6;
        uint8_t output[BytesToRead];
        if (m_i2cBus->writeByteAndReadData(QmcRegister::Data, output, BytesToRead) !=  ESP_OK) return false;
        sample.x = getWord(output);
        sample.y = getWord(output + 2);
        sample.z = getWord(output + 4);
        return true;
    }

    void MagnetoSensorQmc::softReset() {
        m_i2cBus->writeRegister(QmcRegister::Control2, SoftReset);
        static_cast<void>(configure());
    }

    int MagnetoSensorQmc::getNoiseRange() const {
        // only checked on 8 Gauss
        return 60;
    }
}
