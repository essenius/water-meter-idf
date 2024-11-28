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

// Driver for a magnetosensor. Parent of the HMC and QMC variants.
// We don't care a lot about calibration as we're looking for patterns, not for absolute values.
//
// Since these sensors sometimes stops responding, we need a way to hard reset them.
// For that, we simply give it its power from a GPIO port, which we can bring down to reset it.
//
// We also take into account that the sensor needs time to switch on and off

#pragma once

#include <limits.h>

namespace magneto_sensor {
    struct SensorData {
        short x;
        short y;
        short z;

        void reset() {
            x = 0;
            y = 0;
            z = 0;
        }

        bool isSaturated() const {
            return x == SHRT_MIN || y == SHRT_MIN || z == SHRT_MIN;
        }

        bool operator==(const SensorData& other) const {
            return this->x == other.x && this->y == other.y && this->z == other.z;
        }
    };
}
