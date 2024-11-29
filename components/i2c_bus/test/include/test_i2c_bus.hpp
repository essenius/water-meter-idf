#pragma once

namespace i2c {
    void test_is_present();
    void test_read_write_register();
    void test_hmc_read();

    inline void run_tests() {
        RUN_TEST(test_is_present);
        RUN_TEST(test_read_write_register);
        RUN_TEST(test_hmc_read);
    }
}