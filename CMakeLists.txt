cmake_minimum_required(VERSION 3.10)
project(x-ray-controller)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

find_library(WIRINGPI_LIB wiringPi REQUIRED)
find_package(Threads REQUIRED)

add_library(easy_logging STATIC ${SRC_DIR}/easylogging++.cc)

function(add_static_lib name)
    add_library(${name} STATIC ${ARGN})
    target_link_libraries(${name} PRIVATE ${WIRINGPI_LIB} easy_logging)
endfunction()

add_static_lib(gpio_lib    ${SRC_DIR}/gpioController.cpp)
add_static_lib(modbus_lib  ${SRC_DIR}/modbusController.cpp ${SRC_DIR}/uartController.cpp)
add_static_lib(sensor_lib  ${SRC_DIR}/bmp280Controller.cpp ${SRC_DIR}/i2cController.cpp)
add_static_lib(pid_lib     ${SRC_DIR}/pidController.cpp)

add_library(motor_lib STATIC ${SRC_DIR}/motorController.cpp)
target_link_libraries(motor_lib PRIVATE gpio_lib Threads::Threads atomic easy_logging)

add_executable(main ${SRC_DIR}/main.cpp)
target_link_libraries(main PRIVATE gpio_lib modbus_lib sensor_lib motor_lib pid_lib)
