cmake_minimum_required(VERSION 3.16)

project(red_cap_detect LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

# ---- 海康 MVS SDK 路径(按本机实际安装位置修改) ----
set(MVS_ROOT "/opt/MVS")
set(MVS_INCLUDE "${MVS_ROOT}/include")
set(MVS_LIB     "${MVS_ROOT}/lib/64")

add_executable(red_cap_detect
    src/main.cpp
    src/hik_camera.cpp
    src/red_detector.cpp
    src/vision_ui.cpp
)

target_include_directories(red_cap_detect PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${MVS_INCLUDE}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_directories(red_cap_detect PRIVATE ${MVS_LIB})

target_link_libraries(red_cap_detect PRIVATE
    ${OpenCV_LIBS}
    Threads::Threads
    MvCameraControl
)

# 把 MVS 库目录写入 rpath，编译后无需手动 export LD_LIBRARY_PATH 即可直接运行。
set_target_properties(red_cap_detect PROPERTIES
    BUILD_RPATH "${MVS_LIB}"
    INSTALL_RPATH "${MVS_LIB}"
)
