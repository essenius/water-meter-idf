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

// When the magneto-sensor detects a clockwise elliptical move in the X-Y plane, water is flowing.
// The parameters of the ellipse are estimated via a fitting mechanism using a series of samples (see EllipseFit).
// We generate an event every time the cycle moves from the 4th to the 3rd quadrant.
// The detector also tries to filter out anomalies by ignoring points that are too far away from the latest fitted ellipse.

// The signal is disturbed by the presence of electrical devices nearby. Therefore, we sample at 100 Hz (twice the rate of the mains frequency),
// and use a moving average over 4 samples to clean up the signal.

#pragma once

#include <CartesianEllipse.h>
#include <EllipseFit.h>
#include "PubSub.hpp"
#include "SensorSample.hpp"

// needed for compilation in Arduino IDE to define NAN
#include <cmath>

namespace flow_detector {
	using EllipseMath::Angle;
	using EllipseMath::CartesianEllipse;
	using EllipseMath::Coordinate;
	using EllipseMath::EllipseFit;
	using pub_sub::PubSub;
	using pub_sub::Payload;
	using pub_sub::Subscriber;
	using pub_sub::Topic;
	using pub_sub::IntCoordinate;

	class FlowDetector : public pub_sub::Subscriber {
	public:
		FlowDetector(std::shared_ptr<pub_sub::PubSub> pubsub, EllipseFit& ellipseFit);
		void begin(unsigned int noiseRange = 3);
        bool foundAnomaly() const { return m_foundAnomaly; }
		bool foundPulse() const { return m_foundPulse; }
		bool isSearching() const { return m_searchingForPulse; }
		Coordinate getMovingAverage() const { return m_movingAverage; }
        void resetMeasurement();
		void subscriberCallback(const Topic topic, const Payload& payload) override;
		bool wasReset() const { return m_wasReset; }
		bool wasSkipped() const { return m_wasSkipped; }
		SensorSample ellipseCenterTimes10() const { auto center = m_confirmedGoodFit.getCenter(); return SensorSample(IntCoordinate::times10(center.x, center.y)); }
		SensorSample ellipseRadiusTimes10() const { auto radius = m_confirmedGoodFit.getRadius(); return SensorSample(IntCoordinate::times10(radius.x, radius.y)); }
		int16_t ellipseAngleTimes10() const { return m_confirmedGoodFit.getAngle().degreesTimes10(); }
	protected:
		void addSample(const IntCoordinate& sample);
		Coordinate calcMovingAverage();
		void detectPulse(const Coordinate& point);
		CartesianEllipse executeFit() const;
        void waitToSearch(unsigned int quadrant, unsigned int quadrantDifference);
        void findPulseByCenter(const Coordinate& point);
        bool isPulse(const unsigned int quadrant);
        bool startSearching(const unsigned int quadrant) const;
        void findPulseByPrevious(const Coordinate &point);
        bool isOutlier(const Coordinate& point);
        bool isStartingUp(const Coordinate& point);
		bool isRelevant(const Coordinate& point);
		void processMovingAverageSample(const Coordinate& averageSample);
		void reportAnomaly(SensorState state, uint16_t value = 0);
        static int16_t noFitParameter(double angleDistance, bool fitSucceeded);
        void runFirstFit(const Coordinate& point);
        void runNextFit();
        void updateEllipseFit(const Coordinate& point);
		void updateMovingAverageArray(const IntCoordinate& sample);

		static constexpr unsigned int MovingAverageSize = 4;
		static constexpr double MovingAverageNoiseReduction = 2; // = sqrt(MovingAverageSize)
		static constexpr unsigned int MaxConsecutiveOutliers = 50; // half a second

		std::shared_ptr<pub_sub::PubSub> m_pubsub;
		EllipseFit& m_ellipseFit;
		IntCoordinate m_movingAverageArray[MovingAverageSize] = {};
		int8_t m_movingAverageIndex = 0;
		bool m_justStarted = true;
		CartesianEllipse m_confirmedGoodFit;
		unsigned int m_previousQuadrant = 0;
		Coordinate m_startPoint = {};
		Coordinate m_referencePoint = {};

		Coordinate m_previousPoint = {};
		Angle m_startTangent = { NAN };
		unsigned int m_waitCount = 0;
		bool m_searchingForPulse = true;
		Angle m_previousAngleWithCenter = { NAN };
		double m_angleDistanceTravelled = 0;
		bool m_foundAnomaly = false;
		double m_distanceThreshold = 2.12132; // noise range = 3, distance = sqrt(18), MA(4) reduces noise with factor 2
		bool m_firstCall = true;
		bool m_firstRound = true;
		Coordinate m_movingAverage = { NAN, NAN };
		bool m_foundPulse = false;
		bool m_wasSkipped = false;
		double m_tangentDistanceTravelled = 0;
		Angle m_previousAngleWithPreviousFromStart = {};
		bool m_wasReset = true;
        int m_consecutiveOutlierCount = 0;
    };
}
