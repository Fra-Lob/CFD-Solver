
#include "MeshTests.hpp"
#include "TestUtils.hpp"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>

namespace test {

void testGeometry(
    const cfd::Mesh& mesh,
    const cfd::MeshConfig& config)
{
    const std::size_t nCells = mesh.cells.getNumber();
    const std::size_t nFaces = mesh.faces.getNumber();

    const double dx = config.Lx / config.Nx;
    const double dy = config.Ly / config.Ny;
    const double dz = config.Lz / config.Nz;

    const double expectedVolume = dx * dy * dz;
    double totalVolume = 0.0;

    // 1. Check cell geometry
    for (std::size_t cellId = 0; cellId < nCells; ++cellId) {

        const double volume = mesh.cells.getVolume(cellId);
        const auto center = mesh.cells.getCenter(cellId);

        require(std::isfinite(volume) && volume > 0.0,
                "Geometry: non-positive or invalid cell volume");

        require(approx(volume, expectedVolume),
                "Geometry: incorrect cell volume");

        // Analytical center of a Cartesian cell
        const auto indices = mesh.cells.getIndicesFromId(cellId);

        const std::array<double, 3> expectedCenter = {
            (indices[0] + 0.5) * dx,
            (indices[1] + 0.5) * dy,
            (indices[2] + 0.5) * dz
        };

        for (int d = 0; d < 3; ++d) {
            require(std::isfinite(center[d]) &&
                    approx(center[d], expectedCenter[d]),
                    "Geometry: incorrect cell center");
        }

        totalVolume += volume;
    }

    require(approx(totalVolume,
                   config.Lx * config.Ly * config.Lz),
            "Geometry: incorrect total domain volume");

    // 2. Check face geometry
    for (std::size_t faceId = 0; faceId < nFaces; ++faceId) {

        const auto nodes = mesh.faces.getNodes(faceId);
        const auto center = mesh.faces.getCenter(faceId);
        const auto areaVector = mesh.faces.getAreaVector(faceId);
        const double area = mesh.faces.getArea(faceId);

        require(std::isfinite(area) && area > 0.0,
                "Geometry: non-positive or invalid face area");

        // Bounding box of the four face vertices
        std::array<double, 3> minP = {
            config.Lx, config.Ly, config.Lz
        };

        std::array<double, 3> maxP = {
            0.0, 0.0, 0.0
        };

        for (auto nodeId : nodes) {
            auto p = mesh.nodes.getCoordinates(nodeId);

            for (int d = 0; d < 3; ++d) {
                minP[d] = std::min(minP[d], p[d]);
                maxP[d] = std::max(maxP[d], p[d]);
            }
        }

        std::array<double, 3> span = {
            maxP[0] - minP[0],
            maxP[1] - minP[1],
            maxP[2] - minP[2]
        };

        // For a Cartesian face, exactly one span is zero.
        const double expectedArea =
            span[0] * span[1] +
            span[0] * span[2] +
            span[1] * span[2];

        require(approx(area, expectedArea),
                "Geometry: incorrect face area");

        // Face center = midpoint of its bounding box
        for (int d = 0; d < 3; ++d) {
            const double expectedCenter =
                0.5 * (minP[d] + maxP[d]);

            require(std::isfinite(center[d]) &&
                    approx(center[d], expectedCenter),
                    "Geometry: incorrect face center");
        }

        // Area vector must have the correct magnitude
        const double areaMagnitude = std::sqrt(
            areaVector[0] * areaVector[0] +
            areaVector[1] * areaVector[1] +
            areaVector[2] * areaVector[2]
        );

        for (double component : areaVector) {
            require(std::isfinite(component),
                    "Geometry: invalid area vector component");
        }

        require(approx(areaMagnitude, area),
                "Geometry: area vector magnitude mismatch");

        // Check orientation using stored cell centers
        const auto owner = mesh.faces.getOwner(faceId);
        const auto ownerCenter = mesh.cells.getCenter(owner);

        std::array<double, 3> direction{};

        if (mesh.faces.isBoundary(faceId)) {

            for (int d = 0; d < 3; ++d) {
                direction[d] = center[d] - ownerCenter[d];
            }

        } else {

            const auto neighbor = mesh.faces.getNeighbor(faceId);
            const auto neighborCenter =
                mesh.cells.getCenter(neighbor);

            for (int d = 0; d < 3; ++d) {
                direction[d] =
                    neighborCenter[d] - ownerCenter[d];
            }
        }

        double dot = 0.0;

        for (int d = 0; d < 3; ++d) {
            dot += areaVector[d] * direction[d];
        }

        require(dot > 0.0,
                "Geometry: incorrect stored area vector orientation");
    }

    // 3. Check geometric closure for every cell
    for (std::size_t cellId = 0; cellId < nCells; ++cellId) {

        std::array<double, 3> sumAreaVector{0.0, 0.0, 0.0};

        for (auto faceId : mesh.cells.getFaces(cellId)) {

            const auto areaVector = mesh.faces.getAreaVector(faceId);
            const auto owner = mesh.faces.getOwner(faceId);

            // Stored area vector is outward for the owner.
            const double sign = (owner == cellId) ? 1.0 : -1.0;

            for (int d = 0; d < 3; ++d) {
                sumAreaVector[d] += sign * areaVector[d];
            }
        }

        for (int d = 0; d < 3; ++d) {
            require(approx(sumAreaVector[d], 0.0),
                    "Geometry: cell area vectors do not close");
        }
    }

    std::cout
        << "[PASS] Geometry | "
        << config.Nx << "x"
        << config.Ny << "x"
        << config.Nz
        << " | Volume: " << totalVolume
        << " | Cells: " << nCells
        << " | Faces: " << nFaces
        << '\n';
}

}
