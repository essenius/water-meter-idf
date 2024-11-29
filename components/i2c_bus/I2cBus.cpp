#include "I2cBus.hpp"
#include <esp_log.h>
#include <esp_err.h>

namespace i2c {

    constexpr const char* kTag = "I2CBus";

    I2cBus::I2cBus(uint8_t sda, uint8_t scl) {
        i2c_master_bus_config_t busConfig;
        busConfig.i2c_port = I2C_NUM_0;
        busConfig.sda_io_num = static_cast<gpio_num_t>(sda);
        busConfig.scl_io_num = static_cast<gpio_num_t>(scl);
        busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
        busConfig.glitch_ignore_cnt = 7;
        busConfig.intr_priority = 0;
        busConfig.trans_queue_depth = 0; // synchronous operation
        busConfig.flags.enable_internal_pullup = true;
        ESP_ERROR_CHECK(i2c_new_master_bus(&busConfig, &m_busHandle));
    }

    I2cBus::~I2cBus() {
        if (m_device) {
            i2c_master_bus_rm_device(m_device);
        }
        if (m_busHandle) {
            i2c_del_master_bus(m_busHandle);
        }
    }

    bool I2cBus::isDevicePresent() {
        return i2c_master_probe(m_busHandle, m_address, kProbeTimeoutMillis) == ESP_OK;
    }

    esp_err_t I2cBus::beginTransmission(uint8_t address) {
        if (m_device) return ESP_OK;

        if (address != 0) {
            m_address = address;
        }
        i2c_device_config_t deviceConfig;
        deviceConfig.device_address = m_address;
        deviceConfig.scl_speed_hz = kFrequency;
        auto returnValue = i2c_master_bus_add_device(m_busHandle, &deviceConfig, &m_device);
        return returnValue;
    }

    esp_err_t I2cBus::endTransmission() {
        auto returnValue = i2c_master_bus_rm_device(m_device);
        m_device = nullptr;
        return returnValue;
    }

    bool I2cBus::isInitialized() const {
        if (!m_device) {
            return false;
        }
        return true;
    }

    esp_err_t I2cBus::readRegister(uint8_t sensorRegister, uint8_t& value) {
        return performOperation([this, &sensorRegister, &value]() {
            uint8_t data[2];
            data[0] = sensorRegister;
            data[1] = 0;
            auto returnValue = i2c_master_transmit_receive(this->m_device, data, 1, &value, 1, kProbeTimeoutMillis);
            return returnValue;
        });
    }

    esp_err_t I2cBus::writeRegister(uint8_t sensorRegister, uint8_t value) {

        return performOperation([this, &sensorRegister, &value]() {
            uint8_t data[2];
            data[0] = sensorRegister;
            data[1] = value;
            return i2c_master_transmit(m_device, data, 2, kProbeTimeoutMillis);
        });
    }

    esp_err_t I2cBus::readByte(uint8_t& value) const {
        if (!isInitialized()) return ESP_ERR_INVALID_STATE;
        return i2c_master_receive(m_device, &value, 1, kTimeoutMillis);
    }

    esp_err_t I2cBus::writeByteAndReadData(uint8_t input, uint8_t *output, size_t bytesToRead) {
    return performOperation([this, &input, &output, &bytesToRead]() { 
            return i2c_master_transmit_receive(m_device, &input, 1, output, bytesToRead, kTimeoutMillis);
        });
    }

    esp_err_t I2cBus::writeByte(uint8_t value) const {
        if (!isInitialized()) return ESP_ERR_INVALID_STATE;
        return i2c_master_transmit(m_device, &value, 1, kTimeoutMillis);
    }
}
