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

// If we don't detect a sensor, we use this null sensor instead. That makes the code a bit cleaner.

#pragma once

#include "MagnetoSensor.hpp"

namespace magneto_sensor {
    class MagnetoSensorNull : public MagnetoSensor {
    public:
        MagnetoSensorNull() : MagnetoSensor(nullptr, 0) {}

        bool begin() override {
            return false;
        }

        double getGain() const override {
            return 0.0;
        }

        int getNoiseRange() const override {
            return 0;
        }

        bool isOn() override {
            return true;
        }

        bool isReal() const override {
            return false;
        }

        bool read(SensorData& sample) override {
            sample.reset();
            return false;
        }

        void softReset() override {}

        void waitForPowerOff() override {}
    };
}

