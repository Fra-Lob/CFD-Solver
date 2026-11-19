#include <cfd/mesh/GeometryProcessor.hpp>

#include "HexGeometry.hpp"

namespace cfd {
    namespace {
        // ===================================================================================== //
        //                            Exact curvilinear metrics                                  //
        // ===================================================================================== //
        MeshDomain cellBounds(const Mesh& mesh, std::size_t cellId) {
            const auto index = mesh.cells.getIndicesFromId(cellId);
            MeshDomain bounds;
            for (unsigned axis = 0; axis < 3; ++axis) {
                const auto& domain = mesh.getDomain();
                const double step =
                    (domain[2 * axis + 1] - domain[2 * axis]) / mesh.getResolution()[axis];
                bounds[2 * axis] = domain[2 * axis] + index[axis] * step;
                bounds[2 * axis + 1] = domain[2 * axis] + (index[axis] + 1) * step;
            }
            return bounds;
        }

        struct Metric {
            double measure;
            math::Vector3 center;
            math::Vector3 vector{};
        };

        // Integrals of r dr, r^2 dr, sin(theta), sin^2(theta), sin(theta)cos(theta).
        // Vector areas integrate the normal over the curved surface; their magnitude
        // need not equal the scalar area.
        Metric curvedCell(const MeshDomain& b, MeshType type) {
            const double radial2 = (b[1] - b[0]) * (b[1] + b[0]) / 2;
            const double radial3 = (b[1] - b[0]) * (b[1] * b[1] + b[1] * b[0] + b[0] * b[0]) / 3;
            const double angle = b[3] - b[2];
            const double sine = std::sin(b[3]) - std::sin(b[2]);
            const double cosine = std::cos(b[2]) - std::cos(b[3]);
            if (type == MeshType::Cylindrical) {
                const double radius = radial3 / radial2;
                return {radial2 * angle * (b[5] - b[4]),
                        {radius * sine / angle, radius * cosine / angle, (b[4] + b[5]) / 2}};
            }
            const double radial4 = (b[1] - b[0]) * (b[1] + b[0]) * (b[1] * b[1] + b[0] * b[0]) / 4;
            const double sinSquared = angle / 2 - (std::sin(2 * b[3]) - std::sin(2 * b[2])) / 4;
            const double sinCos =
                (std::sin(b[3]) * std::sin(b[3]) - std::sin(b[2]) * std::sin(b[2])) / 2;
            const double azimuth = b[5] - b[4];
            const double radius = radial4 / radial3;
            return {radial3 * cosine * azimuth,
                    {radius * sinSquared / cosine * (std::sin(b[5]) - std::sin(b[4])) / azimuth,
                     radius * sinSquared / cosine * (std::cos(b[4]) - std::cos(b[5])) / azimuth,
                     radius * sinCos / cosine}};
        }

        Metric curvedFace(const MeshDomain& b, MeshType type, unsigned axis, unsigned side) {
            const double fixed = b[2 * axis + side];
            const double radial2 = (b[1] - b[0]) * (b[1] + b[0]) / 2;
            const double radial3 = (b[1] - b[0]) * (b[1] * b[1] + b[1] * b[0] + b[0] * b[0]) / 3;
            const double angle = b[3] - b[2];
            const double sine = std::sin(b[3]) - std::sin(b[2]);
            const double cosine = std::cos(b[2]) - std::cos(b[3]);
            Metric result{};
            if (type == MeshType::Cylindrical) {
                const double height = b[5] - b[4];
                const double z = (b[4] + b[5]) / 2;
                if (axis == 0) {
                    result = {fixed * angle * height,
                              {fixed * sine / angle, fixed * cosine / angle, z},
                              {fixed * height * sine, fixed * height * cosine, 0}};
                } else if (axis == 1) {
                    const double area = (b[1] - b[0]) * height;
                    const double radius = (b[0] + b[1]) / 2;
                    result = {area,
                              {radius * std::cos(fixed), radius * std::sin(fixed), z},
                              {-area * std::sin(fixed), area * std::cos(fixed), 0}};
                } else {
                    const double area = radial2 * angle;
                    const double radius = radial3 / radial2;
                    result = {area,
                              {radius * sine / angle, radius * cosine / angle, fixed},
                              {0, 0, area}};
                }
            } else {
                const double azimuth = b[5] - b[4];
                const double azimuthSine = std::sin(b[5]) - std::sin(b[4]);
                const double azimuthCosine = std::cos(b[4]) - std::cos(b[5]);
                if (axis == 0) {
                    const double sinSquared =
                        angle / 2 - (std::sin(2 * b[3]) - std::sin(2 * b[2])) / 4;
                    const double sinCos =
                        (std::sin(b[3]) * std::sin(b[3]) - std::sin(b[2]) * std::sin(b[2])) / 2;
                    const math::Vector3 integral{sinSquared * azimuthSine,
                                                 sinSquared * azimuthCosine, sinCos * azimuth};
                    const double solidAngle = cosine * azimuth;
                    result = {fixed * fixed * solidAngle,
                              math::productByCoefficient(integral, fixed / solidAngle),
                              math::productByCoefficient(integral, fixed * fixed)};
                } else if (axis == 1) {
                    const double sinTheta = std::sin(fixed), cosTheta = std::cos(fixed);
                    const double radius = radial3 / radial2;
                    result = {radial2 * sinTheta * azimuth,
                              {radius * sinTheta * azimuthSine / azimuth,
                               radius * sinTheta * azimuthCosine / azimuth, radius * cosTheta},
                              {radial2 * sinTheta * cosTheta * azimuthSine,
                               radial2 * sinTheta * cosTheta * azimuthCosine,
                               -radial2 * sinTheta * sinTheta * azimuth}};
                } else {
                    const double area = radial2 * angle;
                    const double radius = radial3 / radial2;
                    result = {area,
                              {radius * cosine / angle * std::cos(fixed),
                               radius * cosine / angle * std::sin(fixed), radius * sine / angle},
                              {-area * std::sin(fixed), area * std::cos(fixed), 0}};
                }
            }
            result.vector = math::productByCoefficient(result.vector, side == 0 ? -1 : 1);
            return result;
        }

        Metric curvedFaceMetric(const Mesh& mesh, std::size_t faceId) {
            constexpr std::array<unsigned, 6> axes{2, 2, 1, 1, 0, 0};
            const auto owner = mesh.faces.getOwner(faceId);
            const auto faces = mesh.cells.getFaces(owner);
            const auto found = std::find(faces.begin(), faces.end(), faceId);
            if (found == faces.end()) {
                throw std::runtime_error("Invalid curved face owner");
            }
            const auto local = static_cast<unsigned>(found - faces.begin());
            return curvedFace(cellBounds(mesh, owner), mesh.getType(), axes[local], local % 2);
        }
    } // namespace

    // Ensure metadata actually describes the structured mesh before using exact formulas.
    void MeshGeometryProcessor::validateCurvedGrid(const Mesh& mesh) {
        if (!mesh.isCurved()) {
            return;
        }
        const auto& n = mesh.getResolution();
        const auto size = mesh.nodeResolution();
        if (mesh.cells.getNx() != n[0] || mesh.cells.getNy() != n[1] ||
            mesh.cells.getNz() != n[2] || mesh.nodes.getNx() != size[0] ||
            mesh.nodes.getNy() != size[1] || mesh.nodes.getNz() != size[2]) {
            throw std::runtime_error("Curved geometry requires matching structured resolution");
        }
        for (std::size_t id = 0; id < mesh.nodes.getNumber(); ++id) {
            const auto index = mesh.nodes.getIndicesFromId(id);
            const auto expected = mesh.point(
                {double(index[0]) / n[0], double(index[1]) / n[1], double(index[2]) / n[2]});
            const auto actual = mesh.nodes.getCoordinates(id);
            const double scale = std::max({math::norm(expected), mesh.getDomain()[1],
                                           mesh.getDomain()[5] - mesh.getDomain()[4]});
            if (math::norm(detail::subtract(actual, expected)) > 1e-12 * scale) {
                throw std::runtime_error("Curved node coordinates disagree with domain metadata");
            }
        }
        for (std::size_t id = 0; id < mesh.cells.getNumber(); ++id) {
            const auto q = mesh.cells.getIndicesFromId(id);
            const auto node = [&](auto i, auto j, auto k) {
                return mesh.nodes.getIdFromIndices(i % size[0], j % size[1], k % size[2]);
            };
            const auto [i, j, k] = q;
            const std::array<std::size_t, 8> expected{node(i, j, k),
                                                      node(i + 1, j, k),
                                                      node(i + 1, j + 1, k),
                                                      node(i, j + 1, k),
                                                      node(i, j, k + 1),
                                                      node(i + 1, j, k + 1),
                                                      node(i + 1, j + 1, k + 1),
                                                      node(i, j + 1, k + 1)};
            if (mesh.cells.getNodes(id) != expected) {
                throw std::runtime_error("Curved cells disagree with structured indexing");
            }
        }
    }

    // Compare stored metrics without rebuilding or modifying a loaded mesh.
    void MeshGeometryProcessor::validateCurvedMetrics(const Mesh& mesh) {
        if (!mesh.isCurved()) {
            return;
        }
        const auto close = [](double a, double b) {
            return std::abs(a - b) <= 1e-11 * std::max(std::abs(a), std::abs(b));
        };
        const auto closeVector = [](const auto& a, const auto& b) {
            return math::norm(detail::subtract(a, b)) <=
                   1e-11 * std::max(math::norm(a), math::norm(b));
        };
        for (std::size_t id = 0; id < mesh.faces.getNumber(); ++id) {
            const auto expected = curvedFaceMetric(mesh, id);
            if (!close(mesh.faces.getArea(id), expected.measure) ||
                !closeVector(mesh.faces.getCenter(id), expected.center) ||
                !closeVector(mesh.faces.getAreaVector(id), expected.vector)) {
                throw std::runtime_error("Inconsistent analytic face metrics at face " +
                                         std::to_string(id));
            }
        }
        for (std::size_t id = 0; id < mesh.cells.getNumber(); ++id) {
            const auto expected = curvedCell(cellBounds(mesh, id), mesh.getType());
            if (!close(mesh.cells.getVolume(id), expected.measure) ||
                !closeVector(mesh.cells.getCenter(id), expected.center)) {
                throw std::runtime_error("Inconsistent analytic cell metrics at cell " +
                                         std::to_string(id));
            }
        }
    }

    // ===================================================================================== //
    //                         Face areas, area vectors and centroids                        //
    // ===================================================================================== //
    void MeshGeometryProcessor::computeFaces(Mesh& mesh) {
        validateCurvedGrid(mesh);
        if (mesh.isCurved()) {
            for (std::size_t faceId = 0; faceId < mesh.faces.getNumber(); ++faceId) {
                const auto metric = curvedFaceMetric(mesh, faceId);
                mesh.faces.setArea(faceId, metric.measure);
                mesh.faces.setCenter(faceId, metric.center);
                mesh.faces.setAreaVector(faceId, metric.vector);
            }
            return;
        }
        for (std::size_t faceId = 0; faceId < mesh.faces.getNumber(); ++faceId) {
            const auto nodes = detail::canonicalFace(mesh.faces.getNodes(faceId));
            const auto firstVertex = mesh.nodes.getCoordinates(nodes[0]);
            double area = 0;
            math::Vector3 areaVector{}, moment{};
            for (unsigned triangleId = 1; triangleId < 3; ++triangleId) {
                const auto secondVertex =
                    detail::subtract(mesh.nodes.getCoordinates(nodes[triangleId]), firstVertex);
                const auto thirdVertex =
                    detail::subtract(mesh.nodes.getCoordinates(nodes[triangleId + 1]), firstVertex);
                const auto vector =
                    math::productByCoefficient(math::vectorProduct(secondVertex, thirdVertex), 0.5);
                const double triangleArea = math::norm(vector);
                if (!std::isfinite(triangleArea) || triangleArea <= 0) {
                    throw std::runtime_error("Degenerate triangle at face " +
                                             std::to_string(faceId));
                }
                area += triangleArea;
                areaVector = math::vectorSum(areaVector, vector);
                moment = math::vectorSum(
                    moment, math::productByCoefficient(math::vectorSum(secondVertex, thirdVertex),
                                                       triangleArea / 3));
            }
            mesh.faces.setArea(faceId, area);
            mesh.faces.setAreaVector(faceId, areaVector);
            mesh.faces.setCenter(
                faceId, math::vectorSum(firstVertex, math::divisionByCoefficient(moment, area)));
        }
    }

    // ===================================================================================== //
    //                         Cell volumes and volumetric centroids                         //
    // ===================================================================================== //
    void MeshGeometryProcessor::computeCells(Mesh& mesh) {
        validateCurvedGrid(mesh);
        if (mesh.isCurved()) {
            for (std::size_t cellId = 0; cellId < mesh.cells.getNumber(); ++cellId) {
                const auto metric = curvedCell(cellBounds(mesh, cellId), mesh.getType());
                mesh.cells.setVolume(cellId, metric.measure);
                mesh.cells.setCenter(cellId, metric.center);
            }
            return;
        }
        for (std::size_t cellId = 0; cellId < mesh.cells.getNumber(); ++cellId) {
            detail::checkHex(mesh, cellId);
            const auto cellNodes = mesh.cells.getNodes(cellId);
            const auto origin = mesh.nodes.getCoordinates(cellNodes[0]);
            math::Vector3 reference{};
            for (auto id : cellNodes) {
                reference = math::vectorSum(
                    reference, detail::subtract(mesh.nodes.getCoordinates(id), origin));
            }
            reference = math::vectorSum(origin, math::divisionByCoefficient(reference, 8));
            double volume = 0;
            math::Vector3 moment{};
            for (auto face : mesh.cells.getFaces(cellId)) {
                const auto nodes = detail::canonicalFace(mesh.faces.getNodes(face));
                const double sign = mesh.faces.getOwner(face) == cellId ? 1. : -1.;
                const auto firstVertex =
                    detail::subtract(mesh.nodes.getCoordinates(nodes[0]), reference);
                for (unsigned triangleId = 1; triangleId < 3; ++triangleId) {
                    const auto secondVertex =
                        detail::subtract(mesh.nodes.getCoordinates(nodes[triangleId]), reference);
                    const auto thirdVertex = detail::subtract(
                        mesh.nodes.getCoordinates(nodes[triangleId + 1]), reference);
                    const double signedVolume =
                        sign *
                        math::scalarProduct(firstVertex,
                                            math::vectorProduct(secondVertex, thirdVertex)) /
                        6;
                    if (!std::isfinite(signedVolume) || signedVolume <= 0) {
                        throw std::runtime_error("Non-star-shaped or inverted triangulated cell " +
                                                 std::to_string(cellId));
                    }
                    volume += signedVolume;
                    moment = math::vectorSum(
                        moment, math::productByCoefficient(
                                    math::vectorSum(firstVertex, secondVertex, thirdVertex),
                                    signedVolume / 4));
                }
            }
            mesh.cells.setVolume(cellId, volume);
            mesh.cells.setCenter(
                cellId, math::vectorSum(reference, math::divisionByCoefficient(moment, volume)));
        }
    }
} // namespace cfd
