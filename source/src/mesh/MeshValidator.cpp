#include <cfd/mesh/GeometryProcessor.hpp>
#include <cfd/mesh/MeshValidator.hpp>

#include <set>

#include "HexGeometry.hpp"

namespace cfd {
    namespace {
        // ===================================================================================== //
        //                                   Validation helpers                                  //
        // ===================================================================================== //
        void require(bool condition, const std::string& message, std::size_t id) {
            if (!condition) {
                throw std::runtime_error("Mesh validation: " + message + " (entity " +
                                         std::to_string(id) + ")");
            }
        }

        bool finite(const math::Vector3& v) {
            return std::all_of(v.begin(), v.end(), [](double x) {
                return std::isfinite(x);
            });
        }

        void dimensions(std::array<std::size_t, 3> d, std::size_t count) {
            if (d == std::array<std::size_t, 3>{0, 0, 0}) {
                return;
            }
            std::size_t result = 1;
            for (auto n : d) {
                if (!n || result > std::numeric_limits<std::size_t>::max() / n) {
                    throw std::runtime_error("Invalid logical dimensions");
                }
                result *= n;
            }
            if (result != count) {
                throw std::runtime_error("Logical dimensions disagree with entity count");
            }
        }
    } // namespace

    // ===================================================================================== //
    //                               Completed mesh validation                               //
    // ===================================================================================== //
    void MeshValidator::validate(const Mesh& mesh) {
        // Entity counts, logical dimensions and finite node coordinates.
        MeshGeometryProcessor::validateCurvedGrid(mesh);
        const auto numberOfNodes = mesh.nodes.getNumber(), numberOfCells = mesh.cells.getNumber(),
                   numberOfFaces = mesh.faces.getNumber();
        if (!numberOfNodes || !numberOfCells || !numberOfFaces) {
            throw std::runtime_error("Mesh validation: empty mesh");
        }
        dimensions({mesh.nodes.getNx(), mesh.nodes.getNy(), mesh.nodes.getNz()}, numberOfNodes);
        dimensions({mesh.cells.getNx(), mesh.cells.getNy(), mesh.cells.getNz()}, numberOfCells);
        for (std::size_t n = 0; n < numberOfNodes; ++n) {
            require(finite(mesh.nodes.getCoordinates(n)), "nonfinite node", n);
        }

        // Unique face topology, adjacency, metrics and boundary classification.
        std::set<std::array<std::size_t, 4>> uniqueFaces;
        std::vector<unsigned> incidences(numberOfFaces, 0);
        for (std::size_t faceId = 0; faceId < numberOfFaces; ++faceId) {
            auto nodes = mesh.faces.getNodes(faceId);
            std::sort(nodes.begin(), nodes.end());
            require(nodes.back() < numberOfNodes &&
                        std::adjacent_find(nodes.begin(), nodes.end()) == nodes.end(),
                    "invalid face nodes", faceId);
            require(uniqueFaces.insert(nodes).second, "duplicate face", faceId);
            const auto owner = mesh.faces.getOwner(faceId),
                       neighbor = mesh.faces.getNeighbor(faceId),
                       patch = mesh.faces.getBoundaryPatchId(faceId);
            require(owner < numberOfCells, "invalid owner", faceId);
            require(neighbor == constants::INVALID_ID ||
                        (neighbor < numberOfCells && neighbor != owner),
                    "invalid neighbor", faceId);
            require(std::isfinite(mesh.faces.getArea(faceId)) && mesh.faces.getArea(faceId) > 0,
                    "invalid area", faceId);
            const auto vector = mesh.faces.getAreaVector(faceId),
                       center = mesh.faces.getCenter(faceId);
            require(finite(vector) && finite(center) && math::norm(vector) > 0,
                    "invalid face geometry", faceId);
            require(math::norm(vector) <= mesh.faces.getArea(faceId) * (1 + 1e-10),
                    "area vector exceeds scalar area", faceId);
            if (neighbor == constants::INVALID_ID) {
                require(patch != constants::INVALID_ID && mesh.getPatchNames().contains(patch),
                        "unclassified boundary patch", faceId);
            } else {
                require(patch == constants::INVALID_ID, "internal face has boundary patch", faceId);
            }
        }

        MeshGeometryProcessor::validateCurvedMetrics(mesh);

        // Cell geometry, reciprocal face references and geometric closure.
        std::set<std::array<std::size_t, 8>> uniqueCells;
        for (std::size_t cellId = 0; cellId < numberOfCells; ++cellId) {
            const auto nodes = mesh.cells.getNodes(cellId);
            auto sortedNodes = nodes;
            std::sort(sortedNodes.begin(), sortedNodes.end());
            require(sortedNodes.back() < numberOfNodes &&
                        std::adjacent_find(sortedNodes.begin(), sortedNodes.end()) ==
                            sortedNodes.end(),
                    "invalid cell nodes", cellId);
            require(uniqueCells.insert(sortedNodes).second, "duplicate cell", cellId);
            if (!mesh.isCurved()) {
                detail::checkHex(mesh, cellId);
            }
            require(std::isfinite(mesh.cells.getVolume(cellId)) && mesh.cells.getVolume(cellId) > 0,
                    "invalid cell volume", cellId);
            require(finite(mesh.cells.getCenter(cellId)), "invalid cell center", cellId);
            const auto faces = mesh.cells.getFaces(cellId);
            auto sortedFaces = faces;
            std::sort(sortedFaces.begin(), sortedFaces.end());
            require(sortedFaces.back() < numberOfFaces &&
                        std::adjacent_find(sortedFaces.begin(), sortedFaces.end()) ==
                            sortedFaces.end(),
                    "invalid cell faces", cellId);
            math::Vector3 areaVectorSum{};
            double totalArea = 0;
            for (unsigned local = 0; local < 6; ++local) {
                const auto faceId = faces[local];
                ++incidences[faceId];
                std::array<std::size_t, 4> expected;
                for (unsigned n = 0; n < 4; ++n) {
                    expected[n] = nodes[detail::localFaces[local][n]];
                }
                std::sort(expected.begin(), expected.end());
                auto actual = mesh.faces.getNodes(faceId);
                std::sort(actual.begin(), actual.end());
                require(actual == expected, "cell-to-face node mismatch", cellId);
                const auto owner = mesh.faces.getOwner(faceId),
                           neighbor = mesh.faces.getNeighbor(faceId);
                require(owner == cellId || neighbor == cellId, "cell-to-face owner mismatch",
                        cellId);
                const double sign = owner == cellId ? 1. : -1.;
                const auto outward =
                    math::productByCoefficient(mesh.faces.getAreaVector(faceId), sign);
                if (!mesh.isCurved()) {
                    const auto direction = detail::subtract(mesh.faces.getCenter(faceId),
                                                            mesh.cells.getCenter(cellId));
                    require(math::scalarProduct(outward, direction) > 0, "inward face normal",
                            faceId);
                    // Each oriented triangle must also point away from the cell center.
                    const auto faceNodes = detail::canonicalFace(mesh.faces.getNodes(faceId));
                    const auto firstVertex = detail::subtract(
                        mesh.nodes.getCoordinates(faceNodes[0]), mesh.cells.getCenter(cellId));
                    for (unsigned t = 1; t < 3; ++t) {
                        const auto secondVertex = detail::subtract(
                            mesh.nodes.getCoordinates(faceNodes[t]), mesh.cells.getCenter(cellId));
                        const auto thirdVertex =
                            detail::subtract(mesh.nodes.getCoordinates(faceNodes[t + 1]),
                                             mesh.cells.getCenter(cellId));
                        require(sign * math::scalarProduct(
                                           firstVertex,
                                           math::vectorProduct(secondVertex, thirdVertex)) >
                                    0,
                                "non-star-shaped face triangle", faceId);
                    }
                }
                areaVectorSum = math::vectorSum(areaVectorSum, outward);
                totalArea += mesh.faces.getArea(faceId);
            }
            require(math::norm(areaVectorSum) <= 1e-10 * totalArea, "area-vector closure failure",
                    cellId);
        }
        for (std::size_t faceId = 0; faceId < numberOfFaces; ++faceId) {
            require(incidences[faceId] == (mesh.faces.isBoundary(faceId) ? 1u : 2u),
                    "wrong face incidence count", faceId);
        }
    }
} // namespace cfd
