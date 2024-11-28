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

#pragma once

#include "I2cBus.hpp"
#include <vector>
#include <cstring>

namespace magneto_sensor {
    class I2cBusMock : public i2c::I2cBus {
    public:
        I2cBusMock();
        ~I2cBusMock() override;
        // test usage only
        void clear();
        void setOutput(const uint8_t* buffer, size_t size);
        esp_err_t beginTransmission(uint8_t address = 0) override;
        esp_err_t endTransmission() override;
        bool isDevicePresent() override;
        bool isInitialized() const override;
        esp_err_t readRegister(uint8_t sensorRegister, uint8_t& value) override;
        esp_err_t writeRegister(uint8_t sensorRegister, uint8_t value) override;
        esp_err_t writeByteAndReadData(uint8_t input, uint8_t* output, size_t bytesToRead) override;

        void setIsDevicePresent(bool isPresent) { m_isDevicePresent = isPresent; }
        void setEndTransmissionTogglePeriod(int period);
        size_t writeMismatchIndex(const uint8_t* buffer, size_t size);

    protected:
        esp_err_t readByte(uint8_t& value) const override;
        esp_err_t writeByte(uint8_t value) const override;

    private:
        bool m_isInitialized = false;
        bool m_isDevicePresent = true;
        mutable std::vector<uint8_t> m_writeBuffer;
        mutable std::vector<uint8_t> m_readBuffer;
        int m_endTransmissionTogglePeriod = 10;
        int m_endTransmissionCounter = 10;
        esp_err_t m_endTransmissionErrorValue = ESP_OK;

        mutable int m_readIndex = 0;
        mutable uint8_t reg[3] = {0x00, 0x01, 0x02};
    };
}