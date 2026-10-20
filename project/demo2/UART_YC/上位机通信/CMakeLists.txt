cmake_minimum_required(VERSION 3.16)
project(uart_vision CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

# 硬件回环自测：杜邦线短接 USB-TTL 的 TX/RX 后运行
add_executable(test_loopback test_loopback.cpp src/serial_port.cpp)
