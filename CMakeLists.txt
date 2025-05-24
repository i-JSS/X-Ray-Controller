cmake_minimum_required(VERSION 3.10)
project(x-ray-controller)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

find_library(WIRINGPI_LIB wiringPi REQUIRED)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(BUILD_DIR ${CMAKE_BINARY_DIR})

add_executable(x-ray-controller ${SRC_DIR}/main.cpp ${SRC_DIR}/uart.cpp)
target_link_libraries(x-ray-controller ${WIRINGPI_LIB})

add_executable(exercicio1 ${SRC_DIR}/exercicio1.cpp)
target_link_libraries(exercicio1 ${WIRINGPI_LIB})

add_executable(exercicio2 ${SRC_DIR}/exercicio2.cpp)
target_link_libraries(exercicio2 ${WIRINGPI_LIB})

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
    COMMENT "Reconfiguring with debug flags"
)
