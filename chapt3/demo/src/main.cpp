#include <cstdio>
#include "math_utils.hpp"

int main() {
    float x = 0.5f, y = 1.2f, z = 3.0f;

    float dist = calculate_3d_dist(x, y, z);  // ← 在这一行打断点，F11 跳进函数内部

    printf("点 (%.1f, %.1f, %.1f) 到原点距离: %.3f 米\n", x, y, z, dist);
    return 0;
}
