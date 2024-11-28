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

// Driver for the HMC5883L sensor.
// Datasheet: see https://cdn-shop.adafruit.com/datasheets/HMC5883L_3-Axis_Digital_Compass_IC.pdf

#pragma once

#include "MagnetoSensor.hpp"

namespace magneto_sensor {
    // this sensor has several ranges that you can configure.

    namespace HmcRange {
        constexpr uint8_t Range0_88 = 0;
        constexpr uint8_t Range1_3 = 0b00100000;
        constexpr uint8_t Range1_9 = 0b01000000;
        constexpr uint8_t Range2_5 = 0b01100000;
        constexpr uint8_t Range4_0 = 0b10000000;
        constexpr uint8_t Range4_7 = 0b10100000;
        constexpr uint8_t Range5_6 = 0b11000000;
        constexpr uint8_t Range8_1 = 0b11100000;
    };

    // The sensor can be configured to return samples at different rates

    namespace HmcRate {
        constexpr uint8_t Rate0_75 = 0;
        constexpr uint8_t Rate1_5 = 0b00000100;
        constexpr uint8_t Rate3_0 = 0b00001000;
        constexpr uint8_t Rate7_5 = 0b00001100;
        constexpr uint8_t Rate15 = 0b00010000;
        constexpr uint8_t Rate30 = 0b00010100;
        constexpr uint8_t Rate75 = 0b00011000;
    };

    // Each sample can be an average of a number of "raw" samples

    namespace HmcOverSampling  {
        constexpr uint8_t Sampling1 = 0;
        constexpr uint8_t Sampling2 = 0b00100000;
        constexpr uint8_t Sampling4 = 0b01000000;
        constexpr uint8_t Sampling8 = 0b01100000;
    };

    namespace HmcRegister {
        constexpr uint8_t ControlA = 0;
        constexpr uint8_t ControlB = 1;
        constexpr uint8_t Mode = 2;
        constexpr uint8_t Data = 3;
        constexpr uint8_t Status = 9;
    };

    namespace HmcBias {
        constexpr uint8_t None = 0;
        constexpr uint8_t Positive = 1;
        constexpr uint8_t Negative = 2;
    };

    namespace HmcMode {
        constexpr uint8_t Continuous = 0;
        constexpr uint8_t Single = 1;
        constexpr uint8_t Idle1 = 2;
        constexpr uint8_t Idle2 = 3;
    };

    class MagnetoSensorHmc final : public MagnetoSensor {
    public:
        MagnetoSensorHmc(i2c::I2cBus* i2cBus, const uint8_t address = DefaultAddress);
        void configureRange(uint8_t range);
        void configureOverSampling(uint8_t overSampling);
        void configureRate(uint8_t rate);
        bool handlePowerOn() override;
        bool increaseRange();
        double getGain() const override;
        uint8_t getRange() const;
        int getNoiseRange() const override;
        static double getGain(uint8_t range);
        bool read(SensorData& sample) override;
        void softReset() override;
        static bool testInRange(const SensorData& sample);
        bool test();

    private:
        static constexpr uint8_t DefaultAddress = 0x1E;
        static constexpr int16_t Saturated = -4096;
        void configure(uint8_t range, uint8_t bias) const;
        void getTestMeasurement(SensorData& reading);
        int16_t getWord(const uint8_t* input) const;
        void startMeasurement() const;

        // 4.7 is not likely to get an overflow, and reasonably accurate
        uint8_t m_range = HmcRange::Range4_7;
        // not really important as we use single measurements
        uint8_t m_rate = HmcRate::Rate75;
        // highest possible, to reduce noise
        uint8_t m_overSampling = HmcOverSampling::Sampling8;
        
    };
}

