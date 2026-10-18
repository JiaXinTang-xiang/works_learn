# ====================================================================
# 可复用骨架：现代 CMakeLists.txt
# 用法：拷到新工程根目录，改 project 名、add_executable 里的源文件列表、
#       需要时打开 find_package / target_link_libraries 即可。
# ====================================================================
cmake_minimum_required(VERSION 3.16)

project(my_project VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 构建类型默认值（不要写死 CMAKE_BUILD_TYPE）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# 第三方库：装了 libopencv-dev 后自动找到（需要时取消注释）
# find_package(OpenCV REQUIRED)

add_executable(app
    src/main.cpp
    src/xxx.cpp
)

target_include_directories(app PRIVATE include)

# 链接库（需要时取消注释）
# target_link_libraries(app PRIVATE ${OpenCV_LIBS})
