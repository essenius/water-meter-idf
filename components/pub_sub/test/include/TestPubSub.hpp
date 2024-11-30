#pragma once


#ifdef ESP_PLATFORM
#define DEFINE_TEST_CASE(test_name) \
    TEST_CASE(#test_name, "[magneto_sensor]") 
#else
#define DEFINE_TEST_CASE(test_name) \
    void test_##test_name()

    namespace pub_sub {
        void test_all_payload_types();
        void test_multiple_subscribers();

        inline void run_tests() {
            RUN_TEST(test_all_payload_types);

            // This test crashes in host_test (not in ESP32) and is disabled for now
            RUN_TEST(test_multiple_subscribers);
        }
    }
#endif