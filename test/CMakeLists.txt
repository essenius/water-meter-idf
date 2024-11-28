# This is the project CMakeLists.txt file for the test subproject
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")
set(SDKCONFIG_DEFAULTS "sdkconfig.defaults")

set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../components" "$ENV{IDF_PATH}/tools/unit-test-app/components")

message(STATUS "CMakeLists.txt: test - CMAKE_SOURCE_DIR=" ${CMAKE_SOURCE_DIR})
message(STATUS "CMakeLists.txt: test - EXTRA_COMPONENT_DIRS=" ${EXTRA_COMPONENT_DIRS})

# Set the components to include the tests for.
# This can be overriden from CMake cache:
# - when invoking CMake directly: cmake -D TEST_COMPONENTS="xxxxx" ..
# - when using idf.py: idf.py -T xxxxx build
#
set(TEST_COMPONENTS "pub_sub" "i2c_bus" "magneto_sensor" CACHE STRING "List of components to test")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(water_meter_test)