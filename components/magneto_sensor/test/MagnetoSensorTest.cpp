// Copyright 2024 Rik Essenius
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
#include "I2cBusMock.hpp"
#include "MagnetoSensorTest.hpp"

namespace magneto_sensor {

        
DEFINE_TEST_CASE(sensor_is_on) {
        I2cBusMock i2cBus;
        MagnetoSensor sensor(&i2cBus);

        i2cBus.setIsDevicePresent(true);
        TEST_ASSERT_TRUE_MESSAGE(sensor.isOn(), "Sensor is on");

        i2cBus.setIsDevicePresent(false);
        TEST_ASSERT_FALSE_MESSAGE(sensor.isOn(), "Sensor is off");

        sensor.waitForPowerOff();

        MagnetoSensor sensor2(nullptr);
        TEST_ASSERT_FALSE_MESSAGE(sensor2.isOn(), "Sensor without bus is off");
    }
}
