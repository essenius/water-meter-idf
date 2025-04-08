#pragma once

#ifndef ESP_PLATFORM
namespace i2c {
    void test_i2c_is_present();
    void test_i2c_read_write_register();
    void test_i2c_hmc_read();

    inline void run_tests() {
        RUN_TEST(test_i2c_is_present);
        RUN_TEST(test_i2c_read_write_register);
        RUN_TEST(test_i2c_hmc_read);
    }
}
#endif