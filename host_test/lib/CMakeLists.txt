
MESSAGE(STATUS "CMakeLists.txt: lib - CMAKE_MODULE_PATH=" - ${CMAKE_MODULE_PATH})
include(tools)
include(FindGit)
find_package(Git)

message(STATUS "PUB_SUB_DIR=" ${PUB_SUB_DIR})
message(STATUS "IDF_PATH=" ${IDF_PATH})
message(STATUS "CMAKE_MODULE_PATH" ${CMAKE_MODULE_PATH})    
set(ellipseFitName ellipsefit) # lower case is important for fetching the library
set(ellipseFitLibName EllipseFit) # the name of the library as defined in the CMakeLists.txt of the library

assertVariableSet(GIT_FOUND)

include (FetchContent)

FetchContent_Declare(
    ${ellipseFitName}
    GIT_REPOSITORY https://github.com/essenius/ellipsefit
    GIT_TAG v0.2.5    
)

#set(esp32MockName esp32-mock) # lower case is important for fetching the library

#FetchContent_Declare(
#    ${esp32MockName}
#    GIT_REPOSITORY https://github.com/essenius/esp32-mock
#    GIT_TAG v0.1.12
#)

file(APPEND ${CMAKE_BINARY_DIR}/output.txt "executing FetchContent_MakeAvailable\n")

FetchContent_MakeAvailable_With_Check(${ellipseFitName})

#FetchContent_MakeAvailable_With_Check(${esp32MockName})

file(GLOB sources_list LIST_DIRECTORIES true *)
foreach(dir ${sources_list})
    IF(IS_DIRECTORY ${dir})
    file(APPEND ${CMAKE_BINARY_DIR}/output.txt "dir=${dir}\n")
    ELSE()
    file(APPEND ${CMAKE_BINARY_DIR}/output.txt "file=${dir}\n")
    ENDIF()
endforeach()

# show all variables. 
file(APPEND ${CMAKE_BINARY_DIR}/output.txt "----- in lib\n")
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
   file(APPEND ${CMAKE_BINARY_DIR}/output.txt "${_variableName}=${${_variableName}}\n")
endforeach()
file(APPEND ${CMAKE_BINARY_DIR}/output.txt "----- end lib\n")
