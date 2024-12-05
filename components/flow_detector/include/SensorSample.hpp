// Copyright 2022-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// The sensors deliver 16 bit int coordinates. So X-Y fits in a 32 bit payload, which we use for events.
// We also model saturation here, using the extreme values of 16-bit integers.
// Sensors are responsible to provide their extremes this way.

#pragma once

#include <climits>
#include "PubSub.hpp"

namespace flow_detector {
    using pub_sub::IntCoordinate;

    enum class SensorState : int8_t {
        None = 0,
        Ok,
        PowerError,
        BeginError,
        ReadError,
        Saturated,
        NeedsHardReset,
        NeedsSoftReset,
        Resetting,
        FlatLine,
        Outlier
    };

    class SensorSample {
    public:
        SensorSample(IntCoordinate coordinate) : m_coordinate(coordinate) {}

        // We reserve SHRT_MIN to indicate saturated values
        // We can't use another field for quality, because we need to transport the coordinate in 32 bits.
        bool isSaturated() const {
            return m_coordinate.x == SHRT_MIN || m_coordinate.y == SHRT_MIN;
        }

        // x = SHRT_MAX indicates an error (neither QMC nor HMC delivers this as a valid value)
        // the Y value delivers the error code
        SensorState state() const {
            if (m_coordinate.x == SHRT_MAX) {
                return static_cast<SensorState>(m_coordinate.y);
            }
            return isSaturated() ? SensorState::Saturated : SensorState::Ok;
        }

        static const char* stateToString(const SensorState state) {
            switch (state) {
                case SensorState::None:
                    return "None";
                case SensorState::Ok:
                    return "Ok";
                case SensorState::PowerError:
                    return "PowerError";
                case SensorState::BeginError:
                    return "BeginError";
                case SensorState::ReadError:
                    return "ReadError";
                case SensorState::Saturated:
                    return "Saturated";
                case SensorState::NeedsHardReset:
                    return "NeedsHardReset";
                case SensorState::NeedsSoftReset:
                    return "NeedsSoftReset";
                case SensorState::Resetting:
                    return "Resetting";
                case SensorState::FlatLine:
                    return "FlatLine";
                case SensorState::Outlier:
                    return "Outlier";
                default:
                    return "Unknown";
            }
        }

        static SensorSample error(const SensorState error) {
            return SensorSample({SHRT_MAX, static_cast<int16_t>(error)});
        }

    private:
        IntCoordinate m_coordinate;
    };
}
