cmake_minimum_required(VERSION 3.16)

project(prop_vision_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认构建模式为兼顾速度与调试的 RelWithDebInfo
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# 查找系统 OpenCV 依赖
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

# 添加可执行目标
add_executable(prop_vision_node
    src/main.cpp
    src/TargetDetector.cpp
    src/PnPSolver.cpp
)

# 目标专属头文件目录
target_include_directories(prop_vision_node PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

# 目标专属链接依赖
target_link_libraries(prop_vision_node PRIVATE
    ${OpenCV_LIBS}
    Threads::Threads
)

# 说明：PingPongFrameBuffer.hpp 与 SerialPort.hpp 均为 Header-only（头文件内联实现），
#       故无需在 add_executable 中单独添加对应的 .cpp 源文件。
