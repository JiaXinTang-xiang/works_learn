cmake_minimum_required(VERSION 3.16)

project(detector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

# 实时检测程序（需要摄像头）
add_executable(detector main.cpp)
target_include_directories(detector PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(detector PRIVATE ${OpenCV_LIBS})

# 无摄像头验证程序（合成图片）
add_executable(test_detector test_detector.cpp)
target_include_directories(test_detector PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(test_detector PRIVATE ${OpenCV_LIBS})
