cmake_minimum_required(VERSION 3.10)
project(x-ray-controller)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)
add_compile_options(-Wall -Wextra)

find_library(WIRINGPI_LIB wiringPi REQUIRED)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

add_library(gpio_lib STATIC ${SRC_DIR}/gpioController.cpp)
add_library(motor_lib STATIC ${SRC_DIR}/motorController.cpp)
add_library(modbus_lib STATIC ${SRC_DIR}/modbusController.cpp ${SRC_DIR}/uartController.cpp)
add_library(sensor_lib STATIC ${SRC_DIR}/bmp280Controller.cpp ${SRC_DIR}/i2cController.cpp)
add_library(pid_lib STATIC ${SRC_DIR}/pidController.cpp)
target_link_libraries(gpio_lib ${WIRINGPI_LIB})
target_link_libraries(motor_lib gpio_lib)

add_executable(main ${SRC_DIR}/main.cpp)
target_link_libraries(main gpio_lib modbus_lib sensor_lib)

add_executable(motor-test ${SRC_DIR}/motor-test.cpp)
target_link_libraries(gpio-test motor_lib)
