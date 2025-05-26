cmake_minimum_required(VERSION 3.10)
project(x-ray-controller)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_library(WIRINGPI_LIB wiringPi REQUIRED)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(BUILD_DIR ${CMAKE_BINARY_DIR})

add_executable(main ${SRC_DIR}/main.cpp ${SRC_DIR}/uartController.cpp ${SRC_DIR}/modbusController.cpp)
target_link_libraries(main ${WIRINGPI_LIB})
