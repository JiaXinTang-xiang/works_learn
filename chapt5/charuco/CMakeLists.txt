cmake_minimum_required(VERSION 3.16)

project(charuco LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(charuco_calibrate charuco_calibrate.cpp)
target_include_directories(charuco_calibrate PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(charuco_calibrate PRIVATE ${OpenCV_LIBS})

include(CTest)
if(BUILD_TESTING)
    add_executable(test_physical_board test_physical_board.cpp)
    target_include_directories(test_physical_board PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(test_physical_board PRIVATE ${OpenCV_LIBS})
    add_test(NAME physical_charuco_layout
             COMMAND test_physical_board "${CMAKE_CURRENT_SOURCE_DIR}/my_charuco.jpg")
endif()
