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

#include "I2cBus.hpp"
#include "SensorData.hpp"

namespace magneto_sensor {

    // not using enum classes as we prefer weak typing to make the code more readable

    class MagnetoSensor {
    public:
        virtual ~MagnetoSensor() = default;
        MagnetoSensor(i2c::I2cBus* i2cBus, const uint8_t address = 0);
        //MagnetoSensor(const MagnetoSensor&) = default;
        //MagnetoSensor(MagnetoSensor&&) = default;
        //MagnetoSensor& operator=(const MagnetoSensor&) = default;
        //MagnetoSensor& operator=(MagnetoSensor&&) = default;

        // Configure the sensor
        virtual bool begin();

        virtual double getGain() const { return 1.0; }

        virtual int getNoiseRange() const { return 1; }

        virtual bool handlePowerOn();

        // returns whether the sensor is active
        virtual bool isOn();

        virtual bool isReal() const {
            return true;
        }

        // read a sample from the sensor
        virtual bool read(SensorData& sample) { return false; }

        void setAddress(uint8_t address);

        // soft reset the sensor
        virtual void softReset() { 
            // implement in children
        }

        virtual void waitForPowerOff();

    protected:
        i2c::I2cBus* m_i2cBus;
        uint8_t m_address;
    };
}
