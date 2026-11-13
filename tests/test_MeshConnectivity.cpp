
#include <cfd/mesh/Mesh.hpp>
#include <cfd/meshio/MeshGenerator.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace {

using Id = std::size_t;
using FaceKey = std::array<Id, 4>;

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

FaceKey sortedFace(FaceKey nodes)
{
    std::sort(nodes.begin(), nodes.end());
    return nodes;
}

void testConnectivity(
    std::size_t nx,
    std::size_t ny,
    std::size_t nz)
{
    cfd::MeshConfig config{
        2.0, 1.0, 0.5,
        nx, ny, nz
    };

    cfd::Mesh mesh = cfd::MeshGenerator::generate(config);
    cfd::MeshConnectivityBuilder::build(mesh);

    const Id expectedCells = nx * ny * nz;
    const Id expectedNodes = (nx + 1) * (ny + 1) * (nz + 1);

    const Id expectedFaces =
        (nx + 1) * ny * nz +
        nx * (ny + 1) * nz +
        nx * ny * (nz + 1);

    const Id expectedBoundaryFaces =
        2 * (nx * ny + nx * nz + ny * nz);

    const Id expectedInternalFaces =
        expectedFaces - expectedBoundaryFaces;

    require(mesh.cells.getNumber() == expectedCells,
            "Incorrect number of cells");

    require(mesh.nodes.getNumber() == expectedNodes,
            "Incorrect number of nodes");

    require(mesh.faces.getNumber() == expectedFaces,
            "Incorrect number of unique faces");

    // For every face, count how many cells reference it.
    std::vector<Id> references(expectedFaces, 0);

    // Detect duplicate faces independently of node ordering.
    std::set<FaceKey> uniqueFaces;

    // 1. Check all cells
    for (Id cellId = 0; cellId < expectedCells; ++cellId)
    {
        const auto cellNodes = mesh.cells.getNodes(cellId);
        const auto cellFaces = mesh.cells.getFaces(cellId);

        // All eight nodes must exist and be distinct.
        std::set<Id> uniqueCellNodes;

        for (Id nodeId : cellNodes) {
            require(nodeId < expectedNodes,
                    "Cell references an invalid node");

            uniqueCellNodes.insert(nodeId);
        }

        require(uniqueCellNodes.size() == 8,
                "Cell contains duplicate nodes");

        // All six faces must exist and be distinct.
        std::set<Id> uniqueCellFaces;

        for (Id faceId : cellFaces)
        {
            require(faceId < expectedFaces,
                    "Cell references an invalid face");

            uniqueCellFaces.insert(faceId);
            ++references[faceId];

            const Id owner = mesh.faces.getOwner(faceId);
            const Id neighbor = mesh.faces.getNeighbor(faceId);

            require(owner == cellId || neighbor == cellId,
                    "Cell-face adjacency is inconsistent");

            // Every node of this face must belong to the cell.
            const auto faceNodes = mesh.faces.getNodes(faceId);

            for (Id nodeId : faceNodes) {
                require(uniqueCellNodes.count(nodeId) == 1,
                        "Face contains a node not belonging to its cell");
            }
        }

        require(uniqueCellFaces.size() == 6,
                "Cell contains duplicate face IDs");
    }

    // 2. Check all faces
    Id boundaryFaces = 0;
    Id internalFaces = 0;

    for (Id faceId = 0; faceId < expectedFaces; ++faceId)
    {
        const auto faceNodes = mesh.faces.getNodes(faceId);

        // Four distinct valid nodes.
        std::set<Id> faceNodeSet;

        for (Id nodeId : faceNodes) {
            require(nodeId < expectedNodes,
                    "Face references an invalid node");

            faceNodeSet.insert(nodeId);
        }

        require(faceNodeSet.size() == 4,
                "Face contains duplicate nodes");

        // No duplicate face connectivity.
        const FaceKey key = sortedFace(faceNodes);

        require(uniqueFaces.insert(key).second,
                "Duplicate face detected");

        const Id owner = mesh.faces.getOwner(faceId);
        const Id neighbor = mesh.faces.getNeighbor(faceId);

        require(owner < expectedCells,
                "Face has invalid owner");

        const bool isBoundary =
            mesh.faces.isBoundary(faceId);

        if (isBoundary)
        {
            ++boundaryFaces;

            require(neighbor == cfd::INVALID_ID,
                    "Boundary face has a valid neighbor");

            require(references[faceId] == 1,
                    "Boundary face must belong to exactly one cell");
        }
        else
        {
            ++internalFaces;

            require(neighbor < expectedCells,
                    "Internal face has invalid neighbor");

            require(owner != neighbor,
                    "Face owner equals neighbor");

            require(references[faceId] == 2,
                    "Internal face must belong to exactly two cells");
        }
    }

    require(boundaryFaces == expectedBoundaryFaces,
            "Incorrect number of boundary faces");

    require(internalFaces == expectedInternalFaces,
            "Incorrect number of internal faces");

    // Each internal face is counted twice, each boundary once.
    require(2 * internalFaces + boundaryFaces == 6 * expectedCells,
            "Global cell-face incidence count is inconsistent");

    std::cout
        << "Mesh " << nx << " x " << ny << " x " << nz
        << " passed: "
        << expectedCells << " cells, "
        << expectedNodes << " nodes, "
        << expectedFaces << " faces ("
        << internalFaces << " internal, "
        << boundaryFaces << " boundary)\n";
}

} // anonymous namespace

int main()
{
    try
    {
        testConnectivity(1, 1, 1);
        testConnectivity(2, 1, 1);
        testConnectivity(2, 2, 2);
        testConnectivity(4, 3, 2);
        testConnectivity(10, 8, 6);

        std::cout << "\nAll mesh connectivity tests passed!\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "TEST FAILED: " << e.what() << '\n';
        return 1;
    }
}
