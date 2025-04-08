cmake_minimum_required(VERSION 3.16)

message(STATUS "CMAKE_SOURCE_DIR=" ${CMAKE_SOURCE_DIR})
message(STATUS "CMAKE_BINARY_DIR=" ${CMAKE_BINARY_DIR})
set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")
set(SDKCONFIG_DEFAULTS "sdkconfig.defaults")
set(IDF_COMPONENT_LOCK_PATH "${CMAKE_BINARY_DIR}/dependencies.lock")

#set(ellipseFitName ellipsefit) # lower case is important for fetching the library
#set(ellipseFitLibName EllipseFit) # the name of the library as defined in the CMakeLists.txt of the library
#set(TOP_LEVEL ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

idf_build_set_property(DEPENDENCIES_LOCK "${CMAKE_BINARY_DIR}/dependencies.lock")

project(water_meter)
