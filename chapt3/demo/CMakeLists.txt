cmake_minimum_required(VERSION 3.16)

project(demo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 构建类型默认值：切勿在别处写死 set(CMAKE_BUILD_TYPE ...)
# RelWithDebInfo = 带调试信息的优化版本（-O2 + 调试符号），既能跑又能断点调试
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

add_executable(demo
    src/main.cpp
    src/math_utils.cpp
)

target_include_directories(demo PRIVATE include)
