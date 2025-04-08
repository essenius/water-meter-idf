#include "I2cBus.hpp"
#include "TestI2cBus.hpp"
#include <unity.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#undef DEFINE_TEST_CASE
#ifdef ESP_PLATFORM
#define DEFINE_TEST_CASE(test_name) \
    TEST_CASE(#test_name, "[i2c_bus]") 
#else
#define DEFINE_TEST_CASE(test_name) \
    void test_##test_name()
#endif

namespace i2c {

    constexpr const char* kI2cTag = "TestI2cBus";

    DEFINE_TEST_CASE(i2c_is_present) {
        // make sure an HMC sensor is on the bus when running this test
        i2c::I2cBus i2cBus;
        i2cBus.setAddress(0x00);
        TEST_ASSERT_EQUAL_MESSAGE(0x00, i2cBus.getAddress(), "Address OK");
        TEST_ASSERT_FALSE_MESSAGE(i2cBus.isDevicePresent(), "Device 0x00 is not present");
        i2cBus.setAddress(0x1e);
        TEST_ASSERT_EQUAL_MESSAGE(0x1e, i2cBus.getAddress(), "Address OK");
        TEST_ASSERT_TRUE_MESSAGE(i2cBus.isDevicePresent(), "Device 0x1e is present");
        ESP_LOGI("test", "Device 0x1e is present");
    }

    DEFINE_TEST_CASE(i2c_read_write_register) {
        // make sure an HMC sensor is on the bus when running this test
        i2c::I2cBus i2cBus;
        i2cBus.setAddress(0x1e);
        uint8_t registerValue1 = 0x00;
        uint8_t registerValue2 = 0xff;
        TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, i2cBus.readRegister(0x00, registerValue1), "Read register 0x00 ok");
        TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, i2cBus.writeRegister(0x00, 0x00), "Write register 0x00 ok");
        TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, i2cBus.readRegister(0x00, registerValue2), "Read register 0x00 2nd time ok");
        TEST_ASSERT_EQUAL_MESSAGE(0, registerValue2, "Read back register 0x00 value is 0");
        TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, i2cBus.writeRegister(0x00, registerValue1), "Wrote old value back");

    }

    DEFINE_TEST_CASE(i2c_hmc_read) {
        // make sure an HMC sensor is on the bus when running this test
        i2c::I2cBus i2cBus;
        i2cBus.setAddress(0x1e);
        i2cBus.writeRegister(0, 0x72); // 8 samples, 75 Hz
        i2cBus.writeRegister(1, 0x80); // 4 Gauss
        i2cBus.writeRegister(2, 1); // single modec
        vTaskDelay(pdMS_TO_TICKS(5));
        i2cBus.writeRegister(2,1); // single mode

        constexpr int BytesToRead = 6;
        uint8_t output[BytesToRead];
        TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, i2cBus.writeByteAndReadData(3, output, BytesToRead), "Read data");
        ESP_LOGD(kI2cTag, "X: %d, Y: %d, Z: %d", output[0] * 0xff + output[1], output[2] * 0xff + output[3], output[4] * 0xff + output[5]);
    }
}
