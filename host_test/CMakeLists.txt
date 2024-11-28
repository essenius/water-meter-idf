cmake_minimum_required(VERSION 3.16)

#set(ellipseFitName ellipsefit) # lower case is important for fetching the library
#set(ellipseFitLibName EllipseFit) # the name of the library as defined in the CMakeLists.txt of the library
#set(TOP_LEVEL ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../components")
set(PUB_SUB_DIR "${EXTRA_COMPONENT_DIRS}/pub_sub")

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/lib;${CMAKE_BINARY_DIR}/lib/cmake")

project(water_meter_host_test)

add_subdirectory(lib)

message(STATUS "PUB_SUB_DIR=" ${PUB_SUB_DIR})

# we need this for unity
set (IDF_PATH $ENV{IDF_PATH} CACHE STRING "Path to the ESP-IDF framework")
string(REPLACE "\\" "/" IDF_PATH ${IDF_PATH})
message(STATUS "IDF_PATH=" ${IDF_PATH})


#set (PUB_SUB_DIR ${CMAKE_SOURCE_DIR}/../components/pub_sub CACHE STRING "Path to the pub_sub component")

# Add the source files for the tests
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/unit_test_main.cpp
    ${PUB_SUB_DIR}/test/test_pub_sub.cpp
)

# Add the source files for the components
set(COMPONENT_SOURCES
    ${PUB_SUB_DIR}/pub_sub.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/mock/include
    ${PUB_SUB_DIR}/include
    ${PUB_SUB_DIR}/test/include
    ${IDF_PATH}/components/unity/include
    ${IDF_PATH}/components/unity/unity/src
)

# Enable code coverage
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Enabling code coverage")
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

# Add the executable for the tests
add_executable(${PROJECT_NAME} ${SOURCES} ${COMPONENT_SOURCES})

# Link against the Unity test framework
# We built this library separately. TODO: Add instructions on how to build this library
target_link_libraries(${PROJECT_NAME} ${IDF_PATH}/components/unity/unity/host_build/build/libunity.a)

# Build the tests to run on a Windows host:
#  mkdir build
#  cd build
#  cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
#  mingw32-make.exe