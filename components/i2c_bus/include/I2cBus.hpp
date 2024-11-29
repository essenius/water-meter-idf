#pragma once

#include <cstdint>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "sdkconfig.h"
#include "esp_log.h"

namespace i2c {
    class I2cBus {
    public:
        I2cBus(uint8_t sda = CONFIG_I2C_SDA, uint8_t scl = CONFIG_I2C_SCL);
        virtual ~I2cBus();

        virtual bool isDevicePresent();

        void setAddress(uint8_t address) { m_address = address; }
        uint8_t getAddress() const { return m_address; }

        virtual esp_err_t beginTransmission(uint8_t address = 0);
        virtual esp_err_t endTransmission();
        virtual esp_err_t readRegister(uint8_t sensorRegister, uint8_t& value);
        virtual esp_err_t writeRegister(uint8_t sensorRegister, uint8_t value);
        virtual esp_err_t writeByteAndReadData(uint8_t input, uint8_t* output, size_t bytesToRead);


    protected:
        // can only call this from child classes (i.e. I2cBusMock)
        explicit I2cBus(const char*) {};

        template <typename Func>
        esp_err_t performOperation(Func&& operation) {
            auto error = beginTransmission();
            ESP_LOGI("i2cBus.hpp", "in performOperation after begin transmission: device = %p", m_device);
            if (error != ESP_OK) return error;
            error = operation();
            if (error != ESP_OK) return error;
            return endTransmission();
        }

        virtual esp_err_t readByte(uint8_t& value) const;
        virtual esp_err_t writeByte(uint8_t value) const;

    private:
        static constexpr uint32_t kFrequency = 100 * 1000;
        static constexpr uint32_t kTimeoutMillis = 10;
        static constexpr uint32_t kProbeTimeoutMillis = 50;

        virtual bool isInitialized() const;

        uint8_t m_address = 0;
        i2c_master_bus_handle_t m_busHandle = nullptr;
        i2c_master_dev_handle_t m_device = nullptr;
    };
}
 