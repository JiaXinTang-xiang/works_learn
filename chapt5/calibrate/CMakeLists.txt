cmake_minimum_required(VERSION 3.16)

project(calibrate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(calibrate calibrate.cpp)
target_include_directories(calibrate PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(calibrate PRIVATE ${OpenCV_LIBS})
