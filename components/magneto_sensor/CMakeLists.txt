set(myHeaders MagnetoSensor.h MagnetoSensorHmc.h MagnetoSensorNull.h MagnetoSensorQmc.h SensorData.h)
set(mySources MagnetoSensor.cpp MagnetoSensorHmc.cpp MagnetoSensorQmc.cpp)

idf_component_register(
    SRCS ${mySources}
    INCLUDE_DIRS "include"
    REQUIRES i2c_bus
)