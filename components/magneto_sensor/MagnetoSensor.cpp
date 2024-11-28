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

#include "I2cBus.hpp"
#include "MagnetoSensor.hpp"
#include <esp_err.h>

namespace magneto_sensor {

    constexpr const char* kTag = "MagnetoSensor";

    MagnetoSensor::MagnetoSensor(i2c::I2cBus *i2cBus, const uint8_t address) : m_i2cBus(i2cBus) {
        setAddress(address);
    }

    bool MagnetoSensor::begin()
    {
        softReset();
        return true;
    }

    bool MagnetoSensor::isOn() {
        if (!m_i2cBus) return false;
        return m_i2cBus->isDevicePresent();
    }

    void MagnetoSensor::setAddress(uint8_t address) {
        m_address = address;
        if (m_i2cBus) {
            m_i2cBus->setAddress(address);
        }
    }

    void MagnetoSensor::waitForPowerOff()
    {
        while (isOn()) {}
    }

    bool MagnetoSensor::handlePowerOn() {
        return true;
    }
}
