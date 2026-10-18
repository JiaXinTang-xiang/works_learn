cmake_minimum_required(VERSION 3.16)

project(color_tuner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(hsv_tuner hsv_tuner.cpp)

target_include_directories(hsv_tuner PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(hsv_tuner PRIVATE ${OpenCV_LIBS})
