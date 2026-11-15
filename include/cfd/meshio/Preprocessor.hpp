#pragma once
#include <cfd/mesh/Mesh.hpp>
#include <filesystem>
#include <iosfwd>
namespace cfd {
class Preprocessor {
public:
    // Builds and validates a complete mesh, stages both outputs, then commits them.
    // Optional progress stream; library callers remain quiet by default.
    static Mesh run(const std::filesystem::path& caseDirectory, std::ostream* progress = nullptr);
};
}
