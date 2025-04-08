cmake_minimum_required(VERSION 3.16)

#set(ellipseFitName ellipsefit) # lower case is important for fetching the library
#set(ellipseFitLibName EllipseFit) # the name of the library as defined in the CMakeLists.txt of the library
#set(TOP_LEVEL ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../components" "${CMAKE_SOURCE_DIR}/../managed_components")
#set(PUB_SUB_DIR "${EXTRA_COMPONENT_DIRS}/pub_sub")

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/lib;${CMAKE_BINARY_DIR}/lib/cmake")

project(water_meter_host_test)

# add_subdirectory(lib)

message(STATUS "EXTRA_COMPONENT_DIRS=" ${EXTRA_COMPONENT_DIRS})

# we need this for unity
set (IDF_PATH $ENV{IDF_PATH} CACHE STRING "Path to the ESP-IDF framework")
string(REPLACE "\\" "/" IDF_PATH ${IDF_PATH})
message(STATUS "IDF_PATH=" ${IDF_PATH})


#set (PUB_SUB_DIR ${CMAKE_SOURCE_DIR}/../components/pub_sub CACHE STRING "Path to the pub_sub component")

set (TEST_SOURCES)
set (COMPONENT_SOURCES)
set (COMPONENT_INCLUDES) 
set (COMPONENT_TESTDATA)
foreach(DIR ${EXTRA_COMPONENT_DIRS})
    file (GLOB_RECURSE TEST_FILES "${DIR}/*/test/*.cpp")
    list(APPEND TEST_SOURCES ${TEST_FILES})
    file(GLOB_RECURSE COMPONENT_FILES "${DIR}/*/*.cpp")
    list(APPEND COMPONENT_SOURCES ${COMPONENT_FILES})
    file(GLOB COMPONENT_INC "${DIR}/*/include" "${DIR}/*/test/include" "${DIR}/*")
    list(APPEND COMPONENT_INCLUDES ${COMPONENT_INC})
    file(GLOB TESTDATA "${DIR}/*/test/testData/*")
    list(APPEND COMPONENT_TESTDATA ${TESTDATA})

    endforeach()
message(STATUS "TEST_SOURCES=" ${TEST_SOURCES})
message(STATUS "COMPONENT_SOURCES=" ${COMPONENT_SOURCES})
message(STATUS "COMPONENT_INCLUDES=" ${COMPONENT_INCLUDES})
message(STATUS "COMPONENT_TESTDATA=" ${COMPONENT_TESTDATA})

file (GLOB_RECURSE MOCK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/mock/*.cpp")
message(STATUS "MOCK_SOURCES=" ${MOCK_SOURCES})

# Add the source files for the tests
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/unit_test_main.cpp
    ${MOCK_SOURCES}
    ${TEST_SOURCES}
)
message(STATUS "SOURCES=" ${SOURCES})

# Add the source files for the components
#set(COMPONENT_SOURCES
#    ${PUB_SUB_DIR}/pub_sub.cpp
#)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/mock/include
    ${COMPONENT_INCLUDES}
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

message(STATUS "Building unit tests for ${COMPONENT_NAME}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")

# Define the test data folder
set(TEST_DATA_FOLDER "${CMAKE_SOURCE_DIR}/testData")

# Define the target directory for the test data files
set(TARGET_TEST_DATA_FOLDER "${CMAKE_BINARY_DIR}/testData")

message(STATUS "Copying test data files to the binaries folder for ${PROJECT_NAME}")

# Add a custom command to copy each test data file
foreach(TESTDATA ${COMPONENT_TESTDATA})
    get_filename_component(FILE_NAME ${TESTDATA} NAME)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${TESTDATA} ${TARGET_TEST_DATA_FOLDER}/${FILE_NAME}
        COMMENT "Copying ${FILE_NAME} to the binaries folder"
    )
endforeach()

# Add a custom target to ensure the test data files are copied
add_custom_target(copy_test_data ALL DEPENDS ${PROJECT_NAME})
# Build the tests to run on a Windows host:
#  mkdir build
#  cd build
#  cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
#  mingw32-make.exe
# Run the test executable from a separate console window