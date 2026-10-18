#include "math_utils.hpp"
#include <cmath>

float calculate_3d_dist(float x, float y, float z) {
    return std::sqrt(x * x + y * y + z * z);
}
