
#include "MeshTests.hpp"
#include "TestUtils.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <set>
#include <vector>

namespace test {

void testConnectivity(
    const cfd::Mesh& mesh,
    const cfd::MeshConfig& config)
{
    using Id = std::size_t;
    using FaceKey = std::array<Id, 4>;

    const Id nx = config.Nx;
    const Id ny = config.Ny;
    const Id nz = config.Nz;

    const Id nNodes = mesh.nodes.getNumber();
    const Id nCells = mesh.cells.getNumber();
    const Id nFaces = mesh.faces.getNumber();

    const Id expectedFaces =
        (nx + 1) * ny * nz +
        nx * (ny + 1) * nz +
        nx * ny * (nz + 1);

    const Id expectedBoundary =
        2 * (nx * ny + nx * nz + ny * nz);

    const Id expectedInternal =
        expectedFaces - expectedBoundary;

    require(nFaces == expectedFaces,
            "Connectivity: incorrect total number of faces");

    std::vector<Id> references(nFaces, 0);
    std::set<FaceKey> uniqueFaces;

    // 1. Cell -> Face connectivity
    for (Id cellId = 0; cellId < nCells; ++cellId) {

        auto cellNodes = mesh.cells.getNodes(cellId);
        auto cellFaces = mesh.cells.getFaces(cellId);

        std::set<Id> nodeSet(
            cellNodes.begin(), cellNodes.end()
        );

        std::set<Id> faceSet(
            cellFaces.begin(), cellFaces.end()
        );

        require(nodeSet.size() == 8,
                "Connectivity: duplicated cell nodes");

        require(faceSet.size() == 6,
                "Connectivity: duplicated cell faces");

        for (Id nodeId : cellNodes) {
            require(nodeId < nNodes,
                    "Connectivity: invalid cell node");
        }

        for (Id faceId : cellFaces) {

            require(faceId < nFaces,
                    "Connectivity: cell references invalid face");

            ++references[faceId];

            const Id owner = mesh.faces.getOwner(faceId);
            const Id neighbor = mesh.faces.getNeighbor(faceId);

            require(owner == cellId || neighbor == cellId,
                    "Connectivity: cell-face adjacency mismatch");

            auto faceNodes = mesh.faces.getNodes(faceId);

            for (Id nodeId : faceNodes) {
                require(nodeSet.contains(nodeId),
                        "Connectivity: face node not in cell");
            }
        }
    }

    // 2. Face -> Cell connectivity
    Id boundaryCount = 0;
    Id internalCount = 0;

    for (Id faceId = 0; faceId < nFaces; ++faceId) {

        auto faceNodes = mesh.faces.getNodes(faceId);

        std::set<Id> nodeSet(
            faceNodes.begin(), faceNodes.end()
        );

        require(nodeSet.size() == 4,
                "Connectivity: duplicated face nodes");

        for (Id nodeId : faceNodes) {
            require(nodeId < nNodes,
                    "Connectivity: invalid face node");
        }

        // Detect duplicate faces independently of ordering
        FaceKey key = faceNodes;
        std::sort(key.begin(), key.end());

        require(uniqueFaces.insert(key).second,
                "Connectivity: duplicate face detected");

        const Id owner = mesh.faces.getOwner(faceId);
        const Id neighbor = mesh.faces.getNeighbor(faceId);

        require(owner < nCells,
                "Connectivity: invalid owner");

        if (mesh.faces.isBoundary(faceId)) {

            ++boundaryCount;

            require(references[faceId] == 1,
                    "Connectivity: boundary face must have one cell");

        } else {

            ++internalCount;

            require(neighbor < nCells,
                    "Connectivity: invalid neighbor");

            require(owner != neighbor,
                    "Connectivity: owner equals neighbor");

            require(references[faceId] == 2,
                    "Connectivity: internal face must have two cells");
        }

        // Check face normal orientation for Cartesian meshes
        auto p0 = mesh.nodes.getCoordinates(faceNodes[0]);
        auto p1 = mesh.nodes.getCoordinates(faceNodes[1]);
        auto p2 = mesh.nodes.getCoordinates(faceNodes[2]);

        std::array<double, 3> a{}, b{}, normal{};
        std::array<double, 3> ownerCenter{};
        std::array<double, 3> direction{};

        for (int d = 0; d < 3; ++d) {
            a[d] = p1[d] - p0[d];
            b[d] = p2[d] - p0[d];
        }

        normal = {
            a[1]*b[2] - a[2]*b[1],
            a[2]*b[0] - a[0]*b[2],
            a[0]*b[1] - a[1]*b[0]
        };

        // Compute owner center from its eight nodes
        for (Id nodeId : mesh.cells.getNodes(owner)) {
            auto p = mesh.nodes.getCoordinates(nodeId);

            for (int d = 0; d < 3; ++d) {
                ownerCenter[d] += p[d] / 8.0;
            }
        }

        if (mesh.faces.isBoundary(faceId)) {

            // Direction from owner center to face center
            std::array<double, 3> faceCenter{};

            for (Id nodeId : faceNodes) {
                auto p = mesh.nodes.getCoordinates(nodeId);

                for (int d = 0; d < 3; ++d) {
                    faceCenter[d] += p[d] / 4.0;
                }
            }

            for (int d = 0; d < 3; ++d) {
                direction[d] = faceCenter[d] - ownerCenter[d];
            }

        } else {

            // Direction from owner center to neighbor center
            std::array<double, 3> neighborCenter{};

            for (Id nodeId : mesh.cells.getNodes(neighbor)) {
                auto p = mesh.nodes.getCoordinates(nodeId);

                for (int d = 0; d < 3; ++d) {
                    neighborCenter[d] += p[d] / 8.0;
                }
            }

            for (int d = 0; d < 3; ++d) {
                direction[d] = neighborCenter[d] - ownerCenter[d];
            }
        }

        double dot = 0.0;

        for (int d = 0; d < 3; ++d) {
            dot += normal[d] * direction[d];
        }

        require(dot > 0.0,
                "Connectivity: incorrect face normal orientation");
    }

    // 3. Global consistency
    require(boundaryCount == expectedBoundary,
            "Connectivity: incorrect boundary face count");

    require(internalCount == expectedInternal,
            "Connectivity: incorrect internal face count");

    require(2 * internalCount + boundaryCount == 6 * nCells,
            "Connectivity: incorrect total cell-face incidences");

    std::cout
        << "[PASS] Connectivity | "
        << nx << "x" << ny << "x" << nz
        << " | Faces: " << nFaces
        << " | Internal: " << internalCount
        << " | Boundary: " << boundaryCount
        << '\n';
}

}

