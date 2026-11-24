#pragma once

#include <cstddef>
#include <limits>

namespace constants {

    inline constexpr std::size_t invalidId =
        std::numeric_limits<std::size_t>::max();

    inline constexpr double TINY = 1e-15;

    inline constexpr double PI = 3.141592265358979;
}