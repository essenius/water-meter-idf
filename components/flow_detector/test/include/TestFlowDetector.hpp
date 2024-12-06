#pragma once

#include "FlowDetector.hpp"
#include "SensorSample.hpp"
#include "PulseTestSubscriber.hpp"
#include <string>

// running a file based test is not easy on ESP32, so skipping that for now
#ifdef ESP_PLATFORM
#define DEFINE_TEST_CASE(test_name) \
    TEST_CASE(#test_name, "[flow_detector]") 

#define DEFINE_FILE_TEST_CASE(test_name) \
    TEST_CASE(#test_name, "[flow_detector_file]") 
#else
#define DEFINE_TEST_CASE(test_name) \
    void test_##test_name()

#define DEFINE_FILE_TEST_CASE(test_name) \
    void test_##test_name()
#endif

namespace flow_detector_test {
    using flow_detector::FlowDetector;
    using EllipseMath::EllipseFit;
    using pub_sub::PubSub;
    using pub_sub::IntCoordinate;

    void test_anomaly1();
    void test_anomaly_values_ignored();
    void test_bi_quadrant();
    void test_sensor_was_reset();
    void test_very_slow_flow();

    inline void run_tests() {
        RUN_TEST(test_anomaly1);
        RUN_TEST(test_anomaly_values_ignored);
        RUN_TEST(test_bi_quadrant);
        RUN_TEST(test_sensor_was_reset);
        printf("Running very slow flow test\n");
        RUN_TEST(test_very_slow_flow);
        printf("Done running very slow flow test\n");
    }

    struct ExpectedResult {
        unsigned int firstPulses = 0;
        unsigned int nextPulses = 0;
        unsigned int anomalies = 0;
        unsigned int noFits = 0;
        int drifts = 0;
    };

}
