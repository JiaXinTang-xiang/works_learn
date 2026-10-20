cmake_minimum_required(VERSION 3.16)

project(chapt7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)   # readExact 里用到 std::this_thread

# 无硬件自测：验证封包/CRC/对齐/解包逻辑
add_executable(test_pack test_pack.cpp)
target_link_libraries(test_pack PRIVATE Threads::Threads)

# 硬件回环自测：需要 USB-TTL 模块短接 TX/RX
add_executable(hw1_loopback hw1_loopback.cpp)
target_link_libraries(hw1_loopback PRIVATE Threads::Threads)

# 校验防护测试：故意篡改字节，验证 CRC8 拦下
add_executable(hw2_corrupt_test hw2_corrupt_test.cpp)
target_link_libraries(hw2_corrupt_test PRIVATE Threads::Threads)
