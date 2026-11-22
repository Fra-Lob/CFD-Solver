#include <cfd/mesh/MeshBuilder.hpp>

#include <algorithm>
#include <cmath>

void MeshBuilder::computeBoundaryPatch(Mesh& mesh) {
    const auto [Lx, Ly, Lz] = mesh.getDomain();
    const auto tolerance = constants::TINY *
        std::max({1.0, std::abs(Lx), std::abs(Ly), std::abs(Lz)});

    for (std::size_t faceId = 0; faceId < mesh.faces.getNumber(); faceId++) {
        if (!mesh.faces.isBoundary(faceId)) continue;

        const auto center = mesh.faces.getCenter(faceId);
        std::size_t patchId = constants::invalidId;
        if (std::abs(center[0]) <= tolerance) {
            patchId = static_cast<std::size_t>(CartesianPatch::XMin);
        } else if (std::abs(center[0] - Lx) <= tolerance) {
            patchId = static_cast<std::size_t>(CartesianPatch::XMax);
        } else if (std::abs(center[1]) <= tolerance) {
            patchId = static_cast<std::size_t>(CartesianPatch::YMin);
        } else if (std::abs(center[1] - Ly) <= tolerance) {
            patchId = static_cast<std::size_t>(CartesianPatch::YMax);
        } else if (std::abs(center[2]) <= tolerance) {
            patchId = static_cast<std::size_t>(CartesianPatch::ZMin);
        } else if (std::abs(center[2] - Lz) <= tolerance) {
            patchId = static_cast<std::size_t>(CartesianPatch::ZMax);
        }
        mesh.faces.setBoundaryPatchId(faceId, patchId);
    }
}