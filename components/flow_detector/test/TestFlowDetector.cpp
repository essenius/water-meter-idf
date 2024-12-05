// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "unity.h"
#include <climits>

#ifndef ESP_PLATFORM
//#include <corecrt_math_defines.h>

// ensure that Windows.h doesn't define min and max, which mess up the std::min and std::max
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <psapi.h>

#include <direct.h>
#endif

#include <fstream>
#include "FlowDetectorDriver.hpp"
#include "PulseTestSubscriber.hpp"
#include "TestFlowDetector.hpp"
#include "MathUtils.h"

namespace flow_detector_test {
	using EllipseMath::EllipseFit;
	using EllipseMath::Coordinate;
	using flow_detector::FlowDetector;
	using pub_sub::PubSub;
	using pub_sub::Topic;
	using pub_sub::Payload;
	using pub_sub::IntCoordinate;
	using flow_detector::SensorState;
	using pub_sub_test::TestSubscriber;
	using flow_detector::SensorSample;
	using pub_sub::IntCoordinate;

	constexpr const char* kTag = "TestFlowDetector";


#ifdef ESP_PLATFORM
	void flowTestWithFile(const std::string& fileName, const ExpectedResult&, const unsigned int = 3, const char* = nullptr) {
		ESP_LOGW(kTag, "Flow test %s skipped on ESP32", fileName.c_str());
#else
	void flowTestWithFile(const std::string& fileName, const ExpectedResult& expectedResult, const unsigned int noiseLimit = 3, const char* outFileName = nullptr) {
		auto pubsub = PubSub::create();
		EllipseFit ellipseFit;
		FlowDetector flowDetector(pubsub, ellipseFit);
		PulseTestSubscriber pulseClient(pubsub, outFileName);
		flowDetector.begin(noiseLimit);
		IntCoordinate measurement{};
		std::ifstream measurements("testData\\" + fileName);
		TEST_ASSERT_TRUE_MESSAGE(measurements.is_open(), "File open");
		measurements.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		auto measurementCount = 0;
		while (measurements >> measurement.x) {
			measurements >> measurement.y;
			measurementCount++;
			pubsub->publish(Topic::Sample, measurement);
		}
		pulseClient.close();
		TEST_ASSERT_EQUAL_MESSAGE(expectedResult.firstPulses, pulseClient.pulses(false), "First Pulses");
		TEST_ASSERT_EQUAL_MESSAGE(expectedResult.nextPulses, pulseClient.pulses(true), "Next Pulses");
		TEST_ASSERT_EQUAL_MESSAGE(expectedResult.anomalies, pulseClient.anomalies(), "Anomalies");
		TEST_ASSERT_EQUAL_MESSAGE(expectedResult.noFits, pulseClient.noFits(), "NoFits");
		TEST_ASSERT_EQUAL_MESSAGE(expectedResult.drifts, pulseClient.drifts(), "Drifts");
#endif
	}

	IntCoordinate getSample(const double sampleNumber, const double samplesPerCycle, const double angleOffsetSample) {
		constexpr double Radius = 10.0L;
		constexpr int16_t XOffset = -100;
		constexpr int16_t YOffset = 100;
		const double angle = (sampleNumber - angleOffsetSample) * M_PI / samplesPerCycle * 2.0;
		return IntCoordinate(
			static_cast<int16_t>(XOffset + round(sin(angle) * Radius)),
			static_cast<int16_t>(YOffset + round(cos(angle) * Radius))
		);
	}

	void expectAnomalyAndSkipped(const FlowDetector& flowDetector, std::shared_ptr<pub_sub::PubSub> pubsub, const int16_t x, const int16_t y) {
		pubsub->publish(Topic::Sample, IntCoordinate(x, y));
		printf("Published %d, %d\n", x, y);
		pubsub->waitForIdle();
		TEST_ASSERT_TRUE_MESSAGE(flowDetector.foundAnomaly(), "Anomaly");
		TEST_ASSERT_TRUE_MESSAGE(flowDetector.wasSkipped(), "Skipped");
	}

#ifndef ESP_PLATFORM

	static long long getMemoryUsage() {
		PROCESS_MEMORY_COUNTERS pmc;
		if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
			return pmc.WorkingSetSize;
		}
		return 0;
	}

	DEFINE_TEST_CASE(memory) {
		constexpr int Tests = 30;
		long long memory[Tests];
		for (auto i = 0; i < Tests; i++) {
			ExpectedResult expectedResult = { 1, 59, 0 };
			flowTestWithFile("60cycles.txt", expectedResult);
			memory[i] = getMemoryUsage();
		}
		int memoryDifferenceCount = 0;
		for (auto i = 1; i < Tests; i++) {
            const auto difference = memory[i] - memory[i - 1];
			if (difference != 0) memoryDifferenceCount++;
        }
		TEST_ASSERT_TRUE_MESSAGE(memoryDifferenceCount < Tests / 10, "Less than 10% differences in memory: " + memoryDifferenceCount);
	}
#endif

	DEFINE_TEST_CASE(anomaly1) {
		EllipseFit ellipseFit;
		auto pubsub = PubSub::create();
		FlowDetectorDriver flowDetector(pubsub, ellipseFit);
		TestSubscriber anomalySubscriber(1);
		pubsub->subscribe(&anomalySubscriber, Topic::Anomaly);
		flowDetector.begin();
		flowDetector.addSample({ SHRT_MAX, static_cast<int8_t>(SensorState::PowerError) });
		pubsub->waitForIdle();
		TEST_ASSERT_EQUAL_MESSAGE(1, anomalySubscriber.getCallCount(), "Anomaly detected");
		TEST_ASSERT_EQUAL_MESSAGE(Topic::Anomaly, anomalySubscriber.getTopic(), "Anomaly topic");
		auto payload = std::get<int>(anomalySubscriber.getPayload());
		TEST_ASSERT_EQUAL_MESSAGE(SensorState::PowerError, payload, "Anomaly value ");
	}

	DEFINE_TEST_CASE(bi_quadrant) {
		EllipseFit ellipseFit;
		auto pubsub = PubSub::create();
		FlowDetectorDriver flowDetector(pubsub, ellipseFit);
		printf("defining subscriber\n");	
		PulseTestSubscriber subscriber(pubsub);
		printf("defined subscriber\n");	
		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			printf("Current working dir: %s\n", cwd);
		} else {
			perror("getcwd() error");
		}
		// Tests all cases where a quadrant may be skipped close to detection of a pulse or a search start
		// First a circle for fitting, then 12 samples that check whether skipping a quadrant is dealt with right
		// Test is similar to flowTestWithFile, but bypasses the moving average generation for simpler testing.

		flowDetector.begin(2);
		Coordinate measurement{};
		std::ifstream measurements("testData/BiQuadrant.txt");
		if (!measurements.is_open()) {
			printf("Test file not found. Skipping test\n");
			return;
		}
		measurements.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		while (measurements >> measurement.x) {
			measurements >> measurement.y;
			printf("Publishing %.2f, %.2f\n", measurement.x, measurement.y);	
			flowDetector.processMovingAverageSample(measurement);
		}
		pubsub->waitForIdle();
		TEST_ASSERT_EQUAL_MESSAGE(1, subscriber.pulses(false), "One pulse from the circle");
		TEST_ASSERT_EQUAL_MESSAGE(4, subscriber.pulses(true), "4 pulses from the crafted test samples");
		TEST_ASSERT_EQUAL_MESSAGE(0, subscriber.anomalies(), "No anomalies");
		TEST_ASSERT_EQUAL_MESSAGE(0, subscriber.noFits(), "Fit worked");
	}

	DEFINE_TEST_CASE(very_slow_flow) {
		ExpectedResult expectedResult = { 1, 0, 0 };
		flowTestWithFile("verySlow.txt", expectedResult);
	}

	DEFINE_TEST_CASE(many_outliers) {
		ExpectedResult expectedResult = { 4, 153, 50, 2, 1 };
		flowTestWithFile("manyOutliers.txt", expectedResult, 3, "manyOutliersResult.txt");
	}

	DEFINE_TEST_CASE(no_flow) {
		ExpectedResult expectedResult = { 0, 0, 0 };
		flowTestWithFile("noise.txt", expectedResult);
	}

	DEFINE_TEST_CASE(fast_flow) {
    	ExpectedResult expected{2, 75, 0, 0, 0}; 
    	flowTestWithFile("fast.txt", expected);
	}

	DEFINE_TEST_CASE(slow_fast_flow) {
		ExpectedResult expected{1, 11, 0, 0, 0}; 
		flowTestWithFile("slowFast.txt", expected);
	}

	DEFINE_TEST_CASE(slow_flow) {
		ExpectedResult expected{1, 1, 0, 0, 0}; 
		flowTestWithFile("slow.txt", expected);
	}

	DEFINE_TEST_CASE(slowest_flow) {
		ExpectedResult expected{1, 0, 0}; 
		flowTestWithFile("slowest.txt", expected);
	}

	DEFINE_TEST_CASE(fast_flow_then_noisy) {
		ExpectedResult expected{2, 3, 0, 0, 0}; 
		flowTestWithFile("fastThenNoisy.txt", expected, 12);
	}

	DEFINE_TEST_CASE(anomaly) {
		ExpectedResult expected{1, 3, 50, 0, 1}; 
		flowTestWithFile("anomaly.txt", expected);
	}

	DEFINE_TEST_CASE(cycles_60) {
		ExpectedResult expected{1, 59, 0}; 
		flowTestWithFile("60cycles.txt", expected);
	}

	DEFINE_TEST_CASE(noise_at_end) {
		ExpectedResult expected{1, 5, 0}; 
		flowTestWithFile("noiseAtEnd.txt", expected);
	}

	DEFINE_TEST_CASE(no_fit) {
		ExpectedResult expected{1, 0, 0, 1}; 
		flowTestWithFile("forceNoFit.txt", expected);
	}
	
	DEFINE_TEST_CASE(flush) {
		ExpectedResult expected{2, 37, 299, 0, 1}; 
		flowTestWithFile("flush.txt", expected, 11);
	}

	DEFINE_TEST_CASE(wrong_outlier) {
		ExpectedResult expected{1, 61, 10, 0, 0}; 
		flowTestWithFile("wrong outliers.txt", expected, 3);
	}

	DEFINE_TEST_CASE(crash) {
		ExpectedResult expected{1, 11, 0, 0, 0}; 
		flowTestWithFile("crash.txt", expected, 3);
	}

	DEFINE_TEST_CASE(sensor_was_reset) {
		EllipseFit ellipseFit;
		auto pubsub = PubSub::create();
		FlowDetector flowDetector(pubsub, ellipseFit);

		flowDetector.begin(3);
		constexpr int Radius = 20;
		for (int pass = 0; pass < 2; pass++) {
			unsigned int skipped = 0;
			TEST_ASSERT_TRUE_MESSAGE(flowDetector.wasReset(), "Flow detector reset at pass " + pass) ;
			for (int i = 0; i < 30; i++) {
				const double angle = i * M_PI / 16.0;
				pubsub->publish(Topic::Sample, IntCoordinate{static_cast <int16_t>(cos(angle) * Radius), static_cast <int16_t>(sin(angle) * Radius)});
				if (flowDetector.wasSkipped()) skipped++;
			}

			TEST_ASSERT_EQUAL_MESSAGE(8u, skipped, "8 values skipped pass" + pass);
			skipped = 0;
			TEST_ASSERT_FALSE_MESSAGE(flowDetector.wasReset(), "Flow detector not reset after adding samples pass " + pass);
			if (pass == 0) {
				pubsub->publish(Topic::SensorWasReset, true);
			}
		}
		pubsub->publish(Topic::Sample, IntCoordinate{Radius, 0});
		TEST_ASSERT_FALSE_MESSAGE(flowDetector.wasReset(), "Flow detector not reset at end");
		TEST_ASSERT_FALSE_MESSAGE(flowDetector.wasSkipped(), "sample not skipped at end");
	}


	DEFINE_TEST_CASE(anomaly_values_ignored) {
		EllipseFit ellipseFit;
		auto pubsub = PubSub::create();
		FlowDetector flowDetector(pubsub, ellipseFit);
		flowDetector.begin(3);
		pubsub->waitForIdle();
		expectAnomalyAndSkipped(flowDetector, pubsub, SHRT_MIN, SHRT_MAX);
		expectAnomalyAndSkipped(flowDetector, pubsub, 0, SHRT_MIN);
		expectAnomalyAndSkipped(flowDetector, pubsub, SHRT_MAX, 0);
		expectAnomalyAndSkipped(flowDetector, pubsub, SHRT_MIN, 0);
		expectAnomalyAndSkipped(flowDetector, pubsub, SHRT_MIN, SHRT_MAX);
	}
}
