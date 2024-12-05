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

// It was empirically established that relative measurements are less reliable (losing slow moves in the noise),
// so now using absolutes measurements. Since absolute values are not entirely predictable, the tactic is to
// follow the clockwise elliptic move that the signal makes. We do this by regularly feeding relevant data points
// to an ellipse fitting algorithm. We then use the angle between the center of the ellipse and the data point
// to detect a cycle: if it is at the lowest Y point (move from quadrant 4 to 3) we count a pulse.
//
// We only include points that are at least the noise distance away from the previously included point.
// By doing that, the impact of noise (and the risk of outliers in angles) is greatly reduced. Further, the algorithm
// can deal much better with slow flow.
//
// To deal with remaining jitter, we start searching at the top of the ellipse, and we stop searching if a pulse was given.
//
// We also use the fitted ellipse to determine outliers: if a point is too far away from the ellipse it is an outlier.
//
// Before we have a good fit, we use the (less accurate) mechanism of taking the angle with the previous data point.
// It should be close to the angle to the center - PI / 2, and since we subtract, the difference of PI / 2 doesn't matter.
// When that moves from quadrant 3 to 2, we have a pulse. we accept the (small) risk that in the first cycle we get an outlier. 

#include "FlowDetector.hpp"
#include <EllipseFit.h>
#include <MathUtils.h>
#include <utility>

namespace flow_detector {
	using EllipseMath::EllipseFit;
	using EllipseMath::CartesianEllipse;
	using pub_sub::PubSub;
	using pub_sub::Topic;
	using pub_sub::Payload;
	using pub_sub::IntCoordinate;

	constexpr double MinCycleForFit = 0.6;

	FlowDetector::FlowDetector(PubSub& pubsub, EllipseFit& ellipseFit) : m_pubsub(pubsub), m_ellipseFit(ellipseFit) {}

	// Public methods

	void FlowDetector::begin(const unsigned int noiseRange) {
		// we assume that the noise range for X and Y is the same.
		// If the distance between two points is beyond this, it is beyond noise
		m_distanceThreshold = sqrt(2.0 * noiseRange * noiseRange) / MovingAverageNoiseReduction;
		m_pubsub.subscribe(this, Topic::Sample);
		m_pubsub.subscribe(this, Topic::SensorWasReset);
	}

    void FlowDetector::resetMeasurement() {
        m_firstCall = true;
        m_wasReset = true;
        m_justStarted = true;
        m_consecutiveOutlierCount = 0;
		m_confirmedGoodFit = CartesianEllipse();
    }

	void FlowDetector::subscriberCallback(const Topic topic, const Payload& payload) {
		if (topic == Topic::Sample) {
			addSample(std::get<IntCoordinate>(payload));
		}
		else if (topic == Topic::SensorWasReset) {
			resetMeasurement();
		}
	}	

	// Private methods

	void FlowDetector::addSample(const IntCoordinate& rawSample) {
		auto sample = SensorSample(rawSample);
		if (const auto state = sample.state(); state != SensorState::Ok) {
			reportAnomaly(state);
			return;
		}
		m_foundAnomaly = false;
		m_wasReset = m_firstCall;
		if (m_firstCall) {
			// skip samples as long as we get a flatline. Happens sometimes just after startup
			if (rawSample.x == 0 && rawSample.y == 0) {
				reportAnomaly(SensorState::FlatLine);
				return;
			}
			m_movingAverageIndex = 0;
			m_firstRound = true;
			m_firstCall = false;
		}
		updateMovingAverageArray(rawSample);
		// if index is 0, we made the first round and the buffer is full. Otherwise, we wait.
		if (m_firstRound && m_movingAverageIndex != 0) {
			m_wasSkipped = true;
			return;
		}

		const auto averageSample = calcMovingAverage();
		processMovingAverageSample(averageSample);
	}


	Coordinate FlowDetector::calcMovingAverage() {
		m_movingAverage = { 0,0 };
		for (const auto i : m_movingAverageArray) {
			m_movingAverage.x += static_cast<double>(i.x);
			m_movingAverage.y += static_cast<double>(i.y);
		}
		m_movingAverage.x /= MovingAverageSize;
		m_movingAverage.y /= MovingAverageSize;
		return m_movingAverage;
	}

	void FlowDetector::detectPulse(const Coordinate& point) {
		if (m_confirmedGoodFit.isValid()) {
			findPulseByCenter(point);
		}
		else {
			findPulseByPrevious(point);
		}
	}

	CartesianEllipse FlowDetector::executeFit() const {
		const auto fittedEllipse = m_ellipseFit.fit();
		const CartesianEllipse returnValue(fittedEllipse);
		m_ellipseFit.begin();
		return returnValue;
	}

    void FlowDetector::waitToSearch(const unsigned int quadrant, const unsigned int quadrantDifference) {
		// Consider the risk that a quadrant gets skipped because of an anomaly
        // start searching at the top of the ellipse. This takes care of jitter
        const auto passedTop =  
			(quadrantDifference == 1 && quadrant == 1) ||
			(quadrantDifference == 2 && (quadrant == 1 || quadrant == 4));
		if (passedTop) {
            m_searchingForPulse = true;
        }
	}

	bool passedBottom(const unsigned int quadrant, const unsigned int quadrantDifference) {
		// Consider the risk that a quadrant gets skipped
		return (quadrantDifference == 1 && quadrant == 3) ||
			(quadrantDifference == 2 && (quadrant == 3 || quadrant == 2));
	}

	void FlowDetector::findPulseByCenter(const Coordinate& point) {
		const auto angleWithCenter = point.getAngleFrom(m_confirmedGoodFit.getCenter());
		const auto quadrant = angleWithCenter.getQuadrant();
		const auto quadrantDifference = (m_previousQuadrant - quadrant) % 4;
		// previous angle is initialized in the first fit, so always has a valid value when coming here
		const auto angleDistance = angleWithCenter - m_previousAngleWithCenter.value;
		m_angleDistanceTravelled += angleDistance;
		if (!m_searchingForPulse) {
			m_foundPulse = false;
			waitToSearch(quadrant, quadrantDifference);
		}
		else {
			// reference point is the bottom of the ellipse
			m_foundPulse = passedBottom(quadrant, quadrantDifference);
			if (m_foundPulse) {
				m_pubsub.publish(Topic::Pulse, true);
				m_searchingForPulse = false;
			}
		}
		m_previousQuadrant = quadrant;
		m_previousAngleWithCenter = angleWithCenter;
	}


	bool FlowDetector::foundPulse(const unsigned int quadrant, const unsigned int previousQuadrant) {
		m_foundPulse = m_searchingForPulse && quadrant == 2 && previousQuadrant == 3;
		return m_foundPulse;
	}

	bool FlowDetector::startSearching(const unsigned int quadrant) const {
		return !m_searchingForPulse && (quadrant == 1 || quadrant == 4);
	}

	void FlowDetector::findPulseByPrevious(const Coordinate& point) {

		const auto angleWithPreviousFromStart = point.getAngleFrom(m_previousPoint) - m_startTangent;
		m_tangentDistanceTravelled += (angleWithPreviousFromStart - m_previousAngleWithPreviousFromStart).value;
		m_previousAngleWithPreviousFromStart = angleWithPreviousFromStart;

		const auto quadrant = point.getAngleFrom(m_previousPoint).getQuadrant();

		// this can be jittery, so use a flag to check whether we counted, and reset the counter at the other side of the ellipse

		if (foundPulse(quadrant, m_previousQuadrant)) {
			m_pubsub.publish(Topic::Pulse, false);
			m_searchingForPulse = false;
		}
	
		if (startSearching(quadrant)) {
			m_searchingForPulse = true;
		}
		m_previousQuadrant = quadrant;
	}

	// We have an outlier if the point is too far away from the confirmed fit.
    bool FlowDetector::isOutlier(const Coordinate& point) {
		const auto distanceFromEllipse = m_confirmedGoodFit.getDistanceFrom(point);
		if (distanceFromEllipse <= m_distanceThreshold * 2) return false;

	    const auto reportedDistance = static_cast<uint16_t>(std::min(lround(distanceFromEllipse * 100), 4095l));
		reportAnomaly(SensorState::Outlier, reportedDistance);
		m_consecutiveOutlierCount++;
		return true;
    }

	// if we have just started, we might have impact from the AC current due to the moving average. Wait until stable.
	// Calculates the start tangent once waited long enough.
    bool FlowDetector::isStartingUp(const Coordinate& point) {
		if (m_justStarted) {
			m_waitCount++;
			if (m_waitCount <= MovingAverageSize) {
				m_wasSkipped = true;
				return true;
			}
			m_startTangent = point.getAngleFrom(m_referencePoint);
			m_justStarted = false;
			m_waitCount = 0;
		}
		return false;
	}

    bool FlowDetector::isRelevant(const Coordinate& point) {

		const auto distance = point.getDistanceFrom(m_referencePoint);
		// if we are too close to the previous point, discard
		if (distance < m_distanceThreshold) {
			m_wasSkipped = true;
			return false;
		}
		if (m_confirmedGoodFit.isValid() && isOutlier(point)) {
    		return false;
		}

		if (isStartingUp(point)) {
			return false;
		}
		m_referencePoint = point;
		return true;
	}

	void FlowDetector::processMovingAverageSample(const Coordinate& averageSample) {
		if (m_firstRound) {
			// We have the first valid moving average. Start the process.
			m_ellipseFit.begin();
			m_startPoint = averageSample;
			m_referencePoint = m_startPoint;
			m_previousPoint = m_startPoint;
			m_firstRound = false;
			m_wasSkipped = true;
			return;
		}

		if (!isRelevant(averageSample)) {
			// not leaving potential loose ends
			m_foundPulse = false;
			// if we have too many outliers in a row, we might have drifted (e.g. the sensor was moved), so we reset the measurement
			if (m_consecutiveOutlierCount > 0 && m_consecutiveOutlierCount % MaxConsecutiveOutliers == 0) {
			    m_pubsub.publish(Topic::Drifted, m_consecutiveOutlierCount);
				resetMeasurement();
			}
			return;
		}
		m_consecutiveOutlierCount = 0;
		detectPulse(averageSample);

		m_ellipseFit.addMeasurement(averageSample);
		if (m_ellipseFit.bufferIsFull()) {
			updateEllipseFit(averageSample);
		}
		m_previousPoint = averageSample;
		m_wasSkipped = false;
	}

	void FlowDetector::reportAnomaly(SensorState state, const uint16_t value) {
		m_foundAnomaly = true;
		m_wasSkipped = true;
		m_pubsub.publish(Topic::Anomaly, static_cast<int16_t>(std::to_underlying(state)) + (value << 4));
	}

	int16_t  FlowDetector::noFitParameter(const double angleDistance, const bool fitSucceeded) {
		return static_cast<int16_t>(round(abs(angleDistance * 180) * (fitSucceeded ? 1.0 : -1.0)));
	}

    void FlowDetector::runFirstFit(const Coordinate& point) {
        const auto fittedEllipse = executeFit();
        const auto center = fittedEllipse.getCenter();
        // number of points per ellipse defines whether the fit is reliable.
        const auto passedCycles = m_tangentDistanceTravelled / (2 * M_PI);
        const auto fitSucceeded = fittedEllipse.isValid();
        if (fitSucceeded && abs(passedCycles) >= MinCycleForFit) {
            m_confirmedGoodFit = fittedEllipse;
            m_previousAngleWithCenter = point.getAngleFrom(center);
            m_previousQuadrant = m_previousAngleWithCenter.getQuadrant();
        }
        else {
            // we need another round
            m_pubsub.publish(Topic::NoFit, noFitParameter(m_tangentDistanceTravelled, fitSucceeded));
        }
        m_tangentDistanceTravelled = 0;
    }

    void FlowDetector::runNextFit() {
        // If we already had a reliable fit, check whether the new data is good enough to warrant a new fit.
        // Otherwise, we keep the old one. 'Good enough' means we covered at least 60% of a cycle.
        // we do this because the ellipse centers are moving a bit, and we want to minimize deviations.
        if (fabs(m_angleDistanceTravelled / (2 * M_PI)) > MinCycleForFit) {
            const auto fittedEllipse = executeFit();
            if (fittedEllipse.isValid()) {
                m_confirmedGoodFit = fittedEllipse;
            }
            else {
                m_pubsub.publish(Topic::NoFit, noFitParameter(m_angleDistanceTravelled, false));
            }
        }
        else {
            // even though we didn't run a fit, we mark it as succeeded to see the difference with one that failed a fit
            m_pubsub.publish(Topic::NoFit, noFitParameter(m_angleDistanceTravelled, true));
            m_ellipseFit.begin();
        }
        m_angleDistanceTravelled = 0;
    }

	void FlowDetector::updateEllipseFit(const Coordinate& point) {
		// The first time we always run a fit. Re-run if the first time(s) didn't result in a good fit
		if (!m_confirmedGoodFit.isValid()) {
			runFirstFit(point);
		}
		else {
			runNextFit();
		}
	}

	void FlowDetector::updateMovingAverageArray(const IntCoordinate& sample) {
		m_movingAverageArray[m_movingAverageIndex] = sample;
		++m_movingAverageIndex %= 4;
	}
}
