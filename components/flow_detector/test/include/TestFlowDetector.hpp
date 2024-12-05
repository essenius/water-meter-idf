#pragma once

#include "FlowDetector.hpp"
#include "SensorSample.hpp"
#include "PulseTestSubscriber.hpp"
#include <string>

#ifdef ESP_PLATFORM
#define DEFINE_TEST_CASE(test_name) \
    TEST_CASE(#test_name, "[flow_detector]") 
#else
#define DEFINE_TEST_CASE(test_name) \
    void test_##test_name()
#endif

namespace flow_detector_test {
    using flow_detector::FlowDetector;
    using EllipseMath::EllipseFit;
    using pub_sub::PubSub;
    using pub_sub::IntCoordinate;

    void test_anomaly1();
    void test_bi_quadrant();
    void test_anomaly_values_ignored();

    inline void run_tests() {
        RUN_TEST(test_anomaly1);
        RUN_TEST(test_anomaly_values_ignored);
        RUN_TEST(test_bi_quadrant);
    }

    struct ExpectedResult {
        unsigned int firstPulses = 0;
        unsigned int nextPulses = 0;
        unsigned int anomalies = 0;
        unsigned int noFits = 0;
        int drifts = 0;
    };

}
