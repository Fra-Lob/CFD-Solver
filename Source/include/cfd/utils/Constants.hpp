#pragma once

#include <cstddef>
#include <limits>

namespace cfd::constants {

    inline constexpr std::size_t invalidId =
        std::numeric_limits<std::size_t>::max();

    inline constexpr double TINY = 1e-15;

}