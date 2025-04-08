#pragma once

#include "FlowDetector.hpp"
#include "SensorSample.hpp"
#include "PulseTestSubscriber.hpp"
#include <string>

#undef DEFINE_TEST_CASE
#undef DEFINE_FILE_TEST_CASE

// running a file based test is not easy on ESP32, so skipping that for now
#ifdef ESP_PLATFORM
#define DEFINE_TEST_CASE(test_name) \
    TEST_CASE(#test_name, "[flow_detector]") 

#define DEFINE_FILE_TEST_CASE(test_name) \
    TEST_CASE(#test_name, "[flow_detector_file]") 
#else
#define DEFINE_TEST_CASE(test_name) \
    void test_flow_##test_name()

#define DEFINE_FILE_TEST_CASE(test_name) \
    void test_flow_##test_name()
#endif

namespace flow_detector_test {
    using flow_detector::FlowDetector;
    using EllipseMath::EllipseFit;
    using pub_sub::PubSub;
    using pub_sub::IntCoordinate;

    void test_flow_anomaly1();
    void test_flow_anomaly_values_ignored();
    void test_flow_bi_quadrant();
    void test_flow_sensor_was_reset();
    void test_flow_very_slow_flow();
    void test_flow_many_outliers();
    void test_flow_no_flow();
    void test_flow_fast_flow();
    void test_flow_slow_fast_flow();
    void test_flow_slow_flow();
    void test_flow_slowest_flow();
    void test_flow_fast_flow_then_noisy();
    void test_flow_anomaly();
    void test_flow_cycles_60();
    void test_flow_noise_at_end();
    void test_flow_no_fit();
    void test_flow_flush();
    void test_flow_wrong_outlier();
    void test_flow_crash();


    inline void run_tests() {
        RUN_TEST(test_flow_anomaly1);
        RUN_TEST(test_flow_anomaly_values_ignored);
        RUN_TEST(test_flow_bi_quadrant);
        RUN_TEST(test_flow_sensor_was_reset);
        printf("Running very slow flow test\n");
        RUN_TEST(test_flow_very_slow_flow);
        printf("Done running very slow flow test\n");
        RUN_TEST(test_flow_many_outliers);
        RUN_TEST(test_flow_no_flow);
        RUN_TEST(test_flow_fast_flow);
        RUN_TEST(test_flow_slow_fast_flow);
        RUN_TEST(test_flow_slow_flow);
        RUN_TEST(test_flow_slowest_flow);
        RUN_TEST(test_flow_fast_flow_then_noisy);
        RUN_TEST(test_flow_anomaly);
        RUN_TEST(test_flow_cycles_60);
        RUN_TEST(test_flow_noise_at_end);
        RUN_TEST(test_flow_no_fit);
        RUN_TEST(test_flow_flush);
        RUN_TEST(test_flow_wrong_outlier);
        RUN_TEST(test_flow_crash);        
    }

    struct ExpectedResult {
        unsigned int firstPulses = 0;
        unsigned int nextPulses = 0;
        unsigned int anomalies = 0;
        unsigned int noFits = 0;
        int drifts = 0;
    };

}
