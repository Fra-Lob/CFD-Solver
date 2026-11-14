
#include "MeshTests.hpp"
#include "TestUtils.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <set>
#include <string>

namespace test {

void testGenerator(
    const cfd::Mesh& mesh,
    const cfd::MeshConfig& config)
{
    const auto nx = config.Nx;
    const auto ny = config.Ny;
    const auto nz = config.Nz;

    const auto expectedNodes = (nx + 1) * (ny + 1) * (nz + 1);
    const auto expectedCells = nx * ny * nz;

    require(mesh.nodes.getNumber() == expectedNodes,
            "Generator: incorrect number of nodes");

    require(mesh.cells.getNumber() == expectedCells,
            "Generator: incorrect number of cells");

    const double dx = config.Lx / nx;
    const double dy = config.Ly / ny;
    const double dz = config.Lz / nz;

    // Check node numbering and coordinates
    for (std::size_t k = 0; k <= nz; ++k) {
        for (std::size_t j = 0; j <= ny; ++j) {
            for (std::size_t i = 0; i <= nx; ++i) {

                const auto nodeId =
                    mesh.nodes.getIdFromIndices(i, j, k);

                const auto expectedId =
                    i + (nx + 1) * (j + (ny + 1) * k);

                require(nodeId == expectedId,
                        "Generator: incorrect node numbering");

                auto p = mesh.nodes.getCoordinates(nodeId);

                require(
                    approx(p[0], i * dx) &&
                    approx(p[1], j * dy) &&
                    approx(p[2], k * dz),
                    "Generator: incorrect node coordinates"
                );

                auto indices = mesh.nodes.getIndicesFromId(nodeId);

                require(
                    indices[0] == i &&
                    indices[1] == j &&
                    indices[2] == k,
                    "Generator: node index conversion failed"
                );
            }
        }
    }

    // Check cell numbering and eight-node connectivity
    for (std::size_t k = 0; k < nz; ++k) {
        for (std::size_t j = 0; j < ny; ++j) {
            for (std::size_t i = 0; i < nx; ++i) {

                const auto cellId =
                    mesh.cells.getIdFromIndices(i, j, k);

                const auto expectedId =
                    i + nx * (j + ny * k);

                require(cellId == expectedId,
                        "Generator: incorrect cell numbering");

                const std::array<std::size_t, 8> expected = {
                    mesh.nodes.getIdFromIndices(i, j, k),
                    mesh.nodes.getIdFromIndices(i + 1, j, k),
                    mesh.nodes.getIdFromIndices(i + 1, j + 1, k),
                    mesh.nodes.getIdFromIndices(i, j + 1, k),
                    mesh.nodes.getIdFromIndices(i, j, k + 1),
                    mesh.nodes.getIdFromIndices(i + 1, j, k + 1),
                    mesh.nodes.getIdFromIndices(i + 1, j + 1, k + 1),
                    mesh.nodes.getIdFromIndices(i, j + 1, k + 1)
                };

                const auto actual = mesh.cells.getNodes(cellId);

                require(actual == expected,
                        "Generator: incorrect cell-node connectivity");

                std::set<std::size_t> uniqueNodes(
                    actual.begin(), actual.end()
                );

                require(uniqueNodes.size() == 8,
                        "Generator: duplicated nodes in cell");

                for (auto nodeId : actual) {
                    require(nodeId < expectedNodes,
                            "Generator: invalid cell node ID");
                }

                auto indices = mesh.cells.getIndicesFromId(cellId);

                require(
                    indices[0] == i &&
                    indices[1] == j &&
                    indices[2] == k,
                    "Generator: cell index conversion failed"
                );
            }
        }
    }

    std::cout
        << "[PASS] Generator | "
        << nx << "x" << ny << "x" << nz
        << " | Nodes: " << expectedNodes
        << " | Cells: " << expectedCells
        << '\n';
}

}
