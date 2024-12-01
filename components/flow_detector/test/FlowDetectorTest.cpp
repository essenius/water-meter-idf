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

#include <gtest/gtest.h>
#include <corecrt_math_defines.h>

// ensure that Windows.h doesn't define min and max, which mess up the std::min and std::max
#define NOMINMAX
#include <Windows.h>
#include <psapi.h>

#include <fstream>
#include "FlowDetectorDriver.h"
#include "PulseTestEventClient.h"

namespace flow_detector_test {
	using EllipseMath::EllipseFit;
	using EllipseMath::Coordinate;
	using flow_detector::FlowDetector;

	class FlowDetectorTest : public testing::Test {
	public:
		static EventServer eventServer;
		static EllipseFit ellipseFit;
	protected:
        static void assertIntCoordinatesEqual(const SensorSample& a, const SensorSample& b, const std::string& label) {
			ASSERT_EQ(a.x, b.x) << label + std::string("(X)");
			ASSERT_EQ(a.y, b.y) << label + std::string("(Y)");
		}

        static void expectResult(const FlowDetector* meter, const wchar_t* description, const int index,
                                 const bool pulse = false, const bool skipped = false, const bool outlier = false) {
			std::wstring message(description);
			message += std::wstring(L" #") + std::to_wstring(index) + std::wstring(L" @ ");
			EXPECT_EQ(pulse, meter->foundPulse()) << message << "Pulse";
			EXPECT_EQ(skipped, meter->wasSkipped()) << message << "Skipped";
			EXPECT_EQ(outlier, meter->foundAnomaly()) << message << "Anomaly";
		}

        static SensorSample getSample(const double sampleNumber, const double samplesPerCycle = 32,
                                       const double angleOffsetSamples = 0) {
			constexpr double Radius = 10.0L;
			constexpr int16_t XOffset = -100;
			constexpr int16_t YOffset = 100;
			const double angle = (sampleNumber - angleOffsetSamples) * M_PI / samplesPerCycle * 2.0;
			return SensorSample{
				{
					static_cast<int16_t>(XOffset + round(sin(angle) * Radius)),
					static_cast<int16_t>(YOffset + round(cos(angle) * Radius))
				}
			};
		}

        static void expectFlowAnalysis(
			const FlowDetectorDriver* actual,
			const std::string& message,
			const int index,
			const SensorSample movingAverage,
			const bool flowStarted = false,
			const bool isPulse = false) {
			assertIntCoordinatesEqual(movingAverage, actual->_movingAverageArray[index], message + " Moving average #" + std::to_string(index));
			EXPECT_EQ(flowStarted, actual->_justStarted) << message << ": just started";
			EXPECT_EQ(isPulse, actual->_foundPulse) << message << ": pulse detected";

		}

		// run process on test signals with a known number of pulses
        static void flowTestWithFile(std::string fileName, const unsigned int firstPulses = 0, const unsigned int nextPulses = 0, const unsigned int anomalies = 0, const unsigned int noFits = 0, const int drifts = 0, const unsigned int noiseLimit = 3, const char* outFileName = nullptr) {
			FlowDetector flowDetector(&eventServer, &ellipseFit);
			PulseTestEventClient pulseClient(&eventServer, outFileName);
			flowDetector.begin(noiseLimit);
			SensorSample measurement{};
			std::ifstream measurements("testData\\" + fileName);
			EXPECT_TRUE(measurements.is_open()) << "File open";
			measurements.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			auto measurementCount = 0;
			while (measurements >> measurement.x) {
				measurements >> measurement.y;
				measurementCount++;
				eventServer.publish(Topic::Sample, measurement);
			}
			pulseClient.close();
			EXPECT_EQ(firstPulses, pulseClient.pulses(false)) << "First Pulses";
			EXPECT_EQ(nextPulses, pulseClient.pulses(true)) << "Next Pulses";
			EXPECT_EQ(anomalies, pulseClient.anomalies()) << "Anomalies";
			EXPECT_EQ(noFits, pulseClient.noFits()) << "NoFits";
			EXPECT_EQ(drifts, pulseClient.drifts()) << "Drifts";
		}

        static void expectAnomalyAndSkipped(const FlowDetector& flowDetector, const int16_t x, const int16_t y) {
			eventServer.publish(Topic::Sample, SensorSample{ {x, y} });
			EXPECT_TRUE(flowDetector.foundAnomaly());
			EXPECT_TRUE(flowDetector.wasSkipped());

		}

		static long long getMemoryUsage() {
			PROCESS_MEMORY_COUNTERS pmc;
			if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
				return pmc.WorkingSetSize;
			}
			return 0;
		}
	};

	PubSub FlowDetectorTest::eventServer;
	EllipseFit FlowDetectorTest::ellipseFit;

	TEST_F(FlowDetectorTest, MemoryTest) {
		constexpr int Tests = 30;
		long long memory[Tests];
		for (auto i = 0; i < Tests; i++) {
			flowTestWithFile("60cycles.txt", 1, 59, 0);
			memory[i] = getMemoryUsage();
		}
		int memoryDifferenceCount = 0;
		for (auto i = 1; i < Tests; i++) {
            const auto difference = memory[i] - memory[i - 1];
			if (difference != 0) memoryDifferenceCount++;
        }
		EXPECT_TRUE(memoryDifferenceCount < Tests / 10) << "Less than 10% differences in memory: " << memoryDifferenceCount;
	}

	TEST_F(FlowDetectorTest, BiQuadrantTest) {
		std::cout << "Current Directory = ";
		system("cd");
		// Tests all cases where a quadrant may be skipped close to detection of a pulse or a search start
		// First a circle for fitting, then 12 samples that check whether skipping a quadrant is dealt with right
		// Test is similar to flowTestWithFile, but bypasses the moving average generation for simpler testing.
		FlowDetectorDriver flowDetector(&eventServer, &ellipseFit);
		const PulseTestEventClient pulseClient(&eventServer);
		flowDetector.begin(2);
		Coordinate measurement{};
		std::ifstream measurements("testData\\BiQuadrant.txt");
		EXPECT_TRUE(measurements.is_open()) << "File open";
		measurements.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		while (measurements >> measurement.x) {
			measurements >> measurement.y;
			flowDetector.processMovingAverageSample(measurement);
		}

		EXPECT_EQ(1, pulseClient.pulses(false)) << "One pulse from the circle";
		EXPECT_EQ(4, pulseClient.pulses(true)) << "4 pulses from the crafted test samples";
		EXPECT_EQ(0, pulseClient.anomalies()) << "No anomalies";
		EXPECT_EQ(0, pulseClient.noFits()) << "Fit worked";
	}

	TEST_F(FlowDetectorTest, VerySlowFlowTest) {
		flowTestWithFile("verySlow.txt", 1,0, 0);
	}

	TEST_F(FlowDetectorTest, ManyOutliersTest) {
		flowTestWithFile("manyOutliers.txt", 4, 153, 50, 2, 1, 3, "manyOutliersResult.txt");
	}

	TEST_F(FlowDetectorTest, NoFlowTest) {
		flowTestWithFile("noise.txt", 0, 0, 0);
	}

	TEST_F(FlowDetectorTest, FastFlowTest) {
		flowTestWithFile("fast.txt", 2, 75, 0);
	}

	TEST_F(FlowDetectorTest, SlowFastFlowTest) {
		flowTestWithFile("slowFast.txt", 1, 11, 0);
	}
	TEST_F(FlowDetectorTest, SlowFlowTest) {
		flowTestWithFile("slow.txt", 1, 1, 0);
	}

	TEST_F(FlowDetectorTest, SlowestFlowTest) {
		flowTestWithFile("slowest.txt", 1, 0, 0);
	}

	TEST_F(FlowDetectorTest, FastFlowThenNoisyTest) {
		// should not trigger on the noisy data
		flowTestWithFile("fastThenNoisy.txt", 2, 3, 0, 0, 0, 12);
	}

	TEST_F(FlowDetectorTest, AnomalyTest) {
		// should not trigger on non-typical movement
		flowTestWithFile("anomaly.txt", 1, 3, 50, 0, 1);
	}

	TEST_F(FlowDetectorTest, 60CyclesTest) {
		flowTestWithFile("60cycles.txt", 1, 59, 0);
	}

	TEST_F(FlowDetectorTest, NoiseAtEndTest) {
		flowTestWithFile("noiseAtEnd.txt", 1, 5, 0);
	}

	TEST_F(FlowDetectorTest, NoFitTest) {
		flowTestWithFile("forceNoFit.txt", 1, 0, 0, 1);
	}

	TEST_F(FlowDetectorTest, FlushTest) {
		flowTestWithFile("flush.txt", 2, 37, 299, 0, 1, 11);
	}

	TEST_F(FlowDetectorTest, WrongOutlierTest) {
		flowTestWithFile("wrong outliers.txt", 1, 61, 10, 0, 0, 3);
	}

	TEST_F(FlowDetectorTest, CrashTest) {
		flowTestWithFile("crash.txt", 1, 11, 0, 0, 0, 3);
	}

	TEST_F(FlowDetectorTest, SensorWasResetTest) {
		FlowDetector flowDetector(&eventServer, &ellipseFit);
		flowDetector.begin(3);
		constexpr int Radius = 20;
		for (int pass = 0; pass < 2; pass++) {
			unsigned int skipped = 0;
			EXPECT_TRUE(flowDetector.wasReset()) << "Flow detector reset at pass " << pass;
			for (int i = 0; i < 30; i++) {
				const double angle = i * M_PI / 16.0;
				eventServer.publish(Topic::Sample, SensorSample{ {static_cast <int16_t>(cos(angle) * Radius), static_cast <int16_t>(sin(angle) * Radius)} });
				if (flowDetector.wasSkipped()) skipped++;
			}

			EXPECT_EQ(8u, skipped) << "8 values skipped pass" << pass;
			skipped = 0;
			EXPECT_FALSE(flowDetector.wasReset()) << "Flow detector not reset after adding samples pass " << pass;
			if (pass == 0) {
				eventServer.publish(Topic::SensorWasReset, true);
			}
		}
		eventServer.publish(Topic::Sample, SensorSample{ {Radius, 0} });
		EXPECT_FALSE(flowDetector.wasReset()) << "Flow detector not reset at end";
		EXPECT_FALSE(flowDetector.wasSkipped()) << "sample not skipped at end";
	}


	TEST_F(FlowDetectorTest, SaturatedValuesIgnoredTest) {
		FlowDetector flowDetector(&eventServer, &ellipseFit);
		flowDetector.begin(3);
		expectAnomalyAndSkipped(flowDetector ,-32768, 32767);
		expectAnomalyAndSkipped(flowDetector, 0, -32768);
		expectAnomalyAndSkipped(flowDetector, 32767, 0);
		expectAnomalyAndSkipped(flowDetector, -32768, 0);
		expectAnomalyAndSkipped(flowDetector, -32768, 32767);
	}
}