#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace test {

inline void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

inline bool approx(double a, double b, double tol = 1e-10)
{
    return std::abs(a - b) <=
           tol * std::max({1.0, std::abs(a), std::abs(b)});
}

}