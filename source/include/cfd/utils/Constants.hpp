#pragma once

#include <cstddef>
#include <limits>

namespace cfd::constants {

    // ===================================================================================== //
    //                               Invalid entity IDs and numerical limits                 //
    // ===================================================================================== //
    inline constexpr std::size_t INVALID_ID = std::numeric_limits<std::size_t>::max();

    inline constexpr double TINY = 1e-15; // minimum difference between 2 equal values

} // namespace cfd::constants
