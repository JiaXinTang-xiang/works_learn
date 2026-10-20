cmake_minimum_required(VERSION 3.16)

project(chapt6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

add_executable(hw1_multithread hw1_multithread.cpp)
target_include_directories(hw1_multithread PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(hw1_multithread PRIVATE ${OpenCV_LIBS} Threads::Threads)
