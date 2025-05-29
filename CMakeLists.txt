cmake_minimum_required(VERSION 3.10)
project(x-ray-controller)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_library(WIRINGPI_LIB wiringPi REQUIRED)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(BUILD_DIR ${CMAKE_BINARY_DIR})

add_library(gpio_lib STATIC ${SRC_DIR}/gpio.cpp ${WIRINGPI_LIB})
add_library(modbus_lib STATIC ${SRC_DIR}/modbusController.cpp ${SRC_DIR}/uartController.cpp)
add_library(sensor_lib STATIC ${SRC_DIR}/bmp280Controller.cpp ${SRC_DIR}/i2cController.cpp)

add_executable(main ${SRC_DIR}/main.cpp)
target_link_libraries(main gpio_lib modbus_lib sensor_lib)

add_executable(gpio-test ${SRC_DIR}/gpio-test.cpp)
target_link_libraries(gpio-test gpio_lib)
