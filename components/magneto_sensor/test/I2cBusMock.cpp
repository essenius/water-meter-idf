#include "I2cBusMock.hpp"
#include "esp_log.h"

namespace magneto_sensor {
    constexpr const char* kTag = "I2cBusMock";

    I2cBusMock::I2cBusMock() : i2c::I2cBus(nullptr) {
        ESP_LOGI(kTag, "Mock Bus Constructor");
    }

    I2cBusMock::~I2cBusMock() {
        ESP_LOGI(kTag, "Mock Bus Destructor");
    }

    void magneto_sensor::I2cBusMock::clear()
    {
        m_writeBuffer.clear();
        m_readBuffer.clear();
        m_readIndex = 0;
        m_endTransmissionTogglePeriod = 10;
        m_endTransmissionCounter = 10;
        m_endTransmissionErrorValue = ESP_OK;
    }

    void I2cBusMock::setOutput(const uint8_t* buffer, size_t size) {
        m_readBuffer.clear();
        m_readBuffer.insert(m_readBuffer.begin(), buffer, buffer + size);
    }

    esp_err_t I2cBusMock::beginTransmission(uint8_t address) {
        if (address != 0) {
            setAddress(address);
        }
        m_isInitialized = true;
        return ESP_OK;
    }

    esp_err_t I2cBusMock::endTransmission() {
        if (m_endTransmissionTogglePeriod == 0) {
            return m_endTransmissionErrorValue;
        }
        if (m_endTransmissionCounter <= 0) {
            m_endTransmissionCounter = m_endTransmissionTogglePeriod;
            m_endTransmissionErrorValue = (m_endTransmissionErrorValue == ESP_OK) ? ESP_FAIL : ESP_OK;
        }
        m_isInitialized = false;
        return m_endTransmissionErrorValue;
    }

bool I2cBusMock::isDevicePresent() {
        return m_isDevicePresent;
        }

    bool I2cBusMock::isInitialized() const {
        return m_isInitialized;
    }

    esp_err_t I2cBusMock::readRegister(uint8_t sensorRegister, uint8_t& value) {
        return performOperation([this, &sensorRegister, &value]() {
            if (sensorRegister >= sizeof(reg)) return ESP_ERR_INVALID_ARG;
            value = reg[sensorRegister];
            return ESP_OK;
        });

    }

    esp_err_t I2cBusMock::writeRegister(uint8_t sensorRegister, uint8_t value) {
        return performOperation([this, &sensorRegister, &value]() {
            writeByte(sensorRegister);
            writeByte(value);
            if (sensorRegister >= sizeof(reg)) return ESP_ERR_INVALID_ARG;
            reg[sensorRegister] = value;
            return ESP_OK;
        });
    }

    esp_err_t I2cBusMock::readByte(uint8_t& value) const {
        if (m_readBuffer.empty()) {
            return ESP_FAIL;
        }
        if (m_readIndex >= m_readBuffer.size()) {
            m_readIndex = 0;
        }
        value = m_readBuffer[m_readIndex++];
        return ESP_OK;
    }

    esp_err_t I2cBusMock::writeByte(uint8_t value) const {
        ESP_LOGI(kTag, "Writing 0x%02x", value);
        m_writeBuffer.push_back(value);
        return ESP_OK;
    }

    esp_err_t I2cBusMock::writeByteAndReadData(uint8_t input, uint8_t* output, size_t bytesToRead) {
        return performOperation([this, &input, output, bytesToRead]() {
            writeByte(input);

            for (size_t i = 0; i < bytesToRead; ++i) {
                if (readByte(output[i]) == ESP_FAIL) {
                    return ESP_FAIL;
                }   
            }
            return ESP_OK;
        });

    }

    void I2cBusMock::setEndTransmissionTogglePeriod(int period) {
        m_endTransmissionTogglePeriod = period;
    }

    size_t I2cBusMock::writeMismatchIndex(const uint8_t* buffer, size_t size) {
        for (size_t i = 0; i < m_writeBuffer.size(); ++i) {
            printf("0x%02x ", m_writeBuffer[i]);
        }
        printf(" (size %d)\n", m_writeBuffer.size());
        for (size_t i = 0; i < size; ++i) {
            printf("0x%02x ", buffer[i]);
        }
        printf(" (size %d)\n", size);
        for (size_t i = 0; i < size; ++i) {
            if (i >= m_writeBuffer.size() || m_writeBuffer[i] != buffer[i]) {
                ESP_LOGI(kTag, "Mismatch at %d", i);
                return i;
            }
        }
        return size;
    }
}