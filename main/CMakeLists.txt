cmake_minimum_required(VERSION 3.16)

idf_component_register(SRCS "main.cpp" "../components/pub_sub/test/TestSubscriber.cpp"
                    INCLUDE_DIRS "." "../components/pub_sub/test/include"
                    REQUIRES ellipse_fit pub_sub magneto_sensor
                    PRIV_REQUIRES nvs_flash)
