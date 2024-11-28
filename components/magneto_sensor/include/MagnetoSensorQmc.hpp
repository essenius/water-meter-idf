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

// Driver for the QMC5883L sensor.
// Datasheet: https://github.com/e-Gizmo/QMC5883L-GY-271-Compass-module/blob/master/QMC5883L%20Datasheet%201.0%20.pdf

#pragma once

#include "MagnetoSensor.hpp"

namespace magneto_sensor {
    // not using enum classes as we prefer weak typing to make the code more readable

    namespace QmcRange {
        constexpr uint8_t Range2G = 0b00000000;
        // divide by 120 for microTesla
        constexpr uint8_t Range8G = 0b00010000;
        // divide by 30 
    }

    namespace QmcRate {
        constexpr uint8_t Rate10Hz = 0b00000000;
        constexpr uint8_t Rate50Hz = 0b00000100;
        constexpr uint8_t Rate100Hz = 0b00001000;
        constexpr uint8_t Rate200Hz = 0b00001100;
    }

    namespace QmcOverSampling {
        constexpr uint8_t Sampling512 = 0b00000000;
        constexpr uint8_t Sampling256 = 0b01000000;
        constexpr uint8_t Sampling128 = 0b10000000;
        constexpr uint8_t Sampling64 = 0b11000000;
    }

    namespace QmcRegister {
        constexpr uint8_t Data = 0x00;
        constexpr uint8_t Status = 0x06;
        constexpr uint8_t Control1 = 0x09;
        constexpr uint8_t Control2 = 0x0A;
        constexpr uint8_t SetReset = 0x0B;
    }

    namespace QmcMode {
        constexpr uint8_t Standby = 0;
        constexpr uint8_t Continuous = 1;
    }

    // QMC5883L sensor driver returning the raw readings.

    class MagnetoSensorQmc final : public MagnetoSensor {
    public:
        MagnetoSensorQmc(i2c::I2cBus* i2cBus, const uint8_t address = DefaultAddress);

        // Configure the sensor according to the configuration parameters (called in begin())
        bool configure() const;

        // configure oversampling if not default (QmcSampling512). Do before begin()
        void configureOverSampling(uint8_t overSampling);

        // configure the range if not default (QmcRange8G). Call before begin()
        void configureRange(uint8_t range);

        // configure the rate if not default (QmcRate100Hz). Call before begin()
        // Note: lower rates won't work with the water meter as the code expects 100 Hz.
        void configureRate(uint8_t rate);

        double getGain() const override;

        static double getGain(uint8_t range);

        uint8_t getRange() const;

        // read a sample from the sensor
        bool read(SensorData& sample) override;

        // soft reset the sensor
        void softReset() override;
        int getNoiseRange() const override;


    private:
        static constexpr uint8_t DefaultAddress = 0x0D;
        
        uint8_t m_overSampling = QmcOverSampling::Sampling512;
        uint8_t m_range = QmcRange::Range8G;
        uint8_t m_rate = QmcRate::Rate100Hz;

        int16_t getWord(const uint8_t* input) const;
    };
}
