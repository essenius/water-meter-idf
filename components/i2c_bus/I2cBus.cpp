#include "I2cBus.hpp"
#include "esp_log.h"
#include "esp_err.h"

namespace i2c {

    constexpr const char* kTag = "I2CBus";

    I2cBus::I2cBus(uint8_t sda, uint8_t scl) {
        ESP_LOGI(kTag, "Creating bus, sda=%d, scl=%d", sda, scl);
        i2c_master_bus_config_t busConfig;
        busConfig.i2c_port = I2C_NUM_0;
        busConfig.sda_io_num = static_cast<gpio_num_t>(sda);
        busConfig.scl_io_num = static_cast<gpio_num_t>(scl);
        busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
        busConfig.glitch_ignore_cnt = 7;
        busConfig.intr_priority = 0;
        busConfig.trans_queue_depth = 0; // synchronous operation
        busConfig.flags.enable_internal_pullup = true;
        ESP_LOGI(kTag, "Creating bus handle");
        ESP_ERROR_CHECK(i2c_new_master_bus(&busConfig, &m_busHandle));
        ESP_LOGI(kTag, "Bus handle %p created", m_busHandle);
    }

    I2cBus::~I2cBus() {
        ESP_LOGI(kTag, "Bus Destructor");
        if (m_device) {
            ESP_LOGI(kTag, "Destructor: removing device %p", m_device);
            i2c_master_bus_rm_device(m_device);
            ESP_LOGI(kTag, "Destructor: removed device %p", m_device);
        }
        if (m_busHandle) {
            ESP_LOGI(kTag, "Destroying bus handle %p", m_busHandle);
            i2c_del_master_bus(m_busHandle);
            ESP_LOGI(kTag, "Destroyed bus handle %p", m_busHandle);
        }
    }

    bool I2cBus::isDevicePresent() {
        ESP_LOGI(kTag, "Probing device 0x%02x on bus %p", m_address, m_busHandle);
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
        ESP_LOGI(kTag, "Adding device 0x%02x to bus %p", m_address, m_busHandle);
        auto returnValue = i2c_master_bus_add_device(m_busHandle, &deviceConfig, &m_device);
        ESP_LOGI(kTag, "Device added, device handle=%p", m_device);
        return returnValue;
    }

    esp_err_t I2cBus::endTransmission() {
        ESP_LOGI(kTag, "Removing device");
        auto returnValue = i2c_master_bus_rm_device(m_device);
        m_device = nullptr;
        return returnValue;
    }

    bool I2cBus::isInitialized() const {
        if (!m_device) {
            ESP_LOGE(kTag, "I2C device not initialized");
            return false;
        }
        return true;
    }

    esp_err_t I2cBus::readRegister(uint8_t sensorRegister, uint8_t& value) {
        return performOperation([this, &sensorRegister, &value]() {
            uint8_t data[2];
            data[0] = sensorRegister;
            data[1] = 0;
            ESP_LOGI(kTag, "Reading register 0x%02x on device %p", sensorRegister, this->m_device);
            auto returnValue = i2c_master_transmit_receive(this->m_device, data, 1, &value, 1, kProbeTimeoutMillis);
            ESP_LOGI(kTag, "Read register 0x%02x value 0x%02x", sensorRegister, value);
            return returnValue;
        });
    }

    esp_err_t I2cBus::writeRegister(uint8_t sensorRegister, uint8_t value) {

        return performOperation([this, &sensorRegister, &value]() {
            uint8_t data[2];
            data[0] = sensorRegister;
            data[1] = value;
            ESP_LOGI(kTag, "Writing 0x%02x to register 0x%02x on device %p", value, sensorRegister, this->m_device);
            return i2c_master_transmit(m_device, data, 2, kProbeTimeoutMillis);
            ESP_LOGI(kTag, "Wrotw 0x%02x to register 0x%02x on device %p", value, sensorRegister, this->m_device);
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
