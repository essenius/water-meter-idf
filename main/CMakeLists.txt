cmake_minimum_required(VERSION 3.16)

idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES ellipse_fit pub_sub magneto_sensor
                    PRIV_REQUIRES nvs_flash)

# we need the source and binaries locations of the libraries that were already built in lib

#foreach (entry ${CMAKE_PREFIX_PATH})
#    list(APPEND includeFolders ${entry}/include)
#endforeach() 

#set(myHeaders "")

#MESSAGE(STATUS "CMakeLists.txt: main - includeFolders=" - ${includeFolders})
                                                                                                                        
#target_link_libraries(${COMPONENT_LIB} PUBLIC EllipseFit)
#target_include_directories(${COMPONENT_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${includeFolders})
                    
#install(TARGETS water_meter DESTINATION lib) # ${CMAKE_CURRENT_BINARY_DIR})
#install(FILES ${myHeaders} DESTINATION include) 


#espressif/button: "=*"
#dht: 
#  path: components/dht
#  git: https://github.com/UncleRus/esp-idf-lib.git
#esp_idf_lib_helpers: 
#  path: components/esp_idf_lib_helpers
#  git: https://github.com/UncleRus/esp-idf-lib.git