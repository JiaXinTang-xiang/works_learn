cmake_minimum_required(VERSION 3.16)

project(pnp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

# 实时 PnP 联调程序（需要 camera_params.yaml + 摄像头）
add_executable(pnp main.cpp)
target_include_directories(pnp PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(pnp PRIVATE ${OpenCV_LIBS})

# 无摄像头验证程序（合成投影 → 还原）
add_executable(test_pnp test_pnp.cpp)
target_include_directories(test_pnp PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(test_pnp PRIVATE ${OpenCV_LIBS})
