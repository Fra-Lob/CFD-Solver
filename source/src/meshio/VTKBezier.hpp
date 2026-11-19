#pragma once

#include <cfd/mesh/GeometryProcessor.hpp>

namespace cfd::detail {
    // VTK tensor-quadratic ordering: vertices, edges, faces, interior.
    inline constexpr std::array<std::array<unsigned, 3>, 27> bezierOffsets{
        {{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}, {0, 0, 2}, {2, 0, 2}, {2, 2, 2},
         {0, 2, 2}, {1, 0, 0}, {2, 1, 0}, {1, 2, 0}, {0, 1, 0}, {1, 0, 2}, {2, 1, 2},
         {1, 2, 2}, {0, 1, 2}, {0, 0, 1}, {2, 0, 1}, {2, 2, 1}, {0, 2, 1}, {0, 1, 1},
         {2, 1, 1}, {1, 0, 1}, {1, 2, 1}, {1, 1, 0}, {1, 1, 2}, {1, 1, 1}}};

    // Shared control grid: neighboring cells reference identical points and weights.
    // Control points are not extra solver nodes. Angular middle controls lie at the
    // intersection of endpoint tangents; weight cos(half angle) gives an exact arc.
    struct BezierGrid {
        std::vector<math::Vector3> points;
        std::vector<double> weights;
        std::vector<std::array<std::size_t, 27>> cells;

        explicit BezierGrid(const Mesh& mesh) {
            MeshGeometryProcessor::validateCurvedGrid(mesh);
            const auto& n = mesh.getResolution();
            const auto& domain = mesh.getDomain();
            std::array<std::size_t, 3> size;
            std::size_t count = 1;
            for (unsigned axis = 0; axis < 3; ++axis) {
                if (n[axis] > (std::numeric_limits<std::size_t>::max() - 1) / 2) {
                    throw std::runtime_error("Bezier grid resolution overflow");
                }
                size[axis] = 2 * n[axis] + (mesh.isPeriodic(axis) ? 0 : 1);
                if (count > std::numeric_limits<std::size_t>::max() / size[axis]) {
                    throw std::runtime_error("Bezier grid point count overflow");
                }
                count *= size[axis];
            }
            points.reserve(count);
            weights.reserve(count);
            const auto id = [&](auto i, auto j, auto k) {
                return i % size[0] + size[0] * (j % size[1] + size[1] * (k % size[2]));
            };
            for (std::size_t k = 0; k < size[2]; ++k) {
                for (std::size_t j = 0; j < size[1]; ++j) {
                    for (std::size_t i = 0; i < size[0]; ++i) {
                        const auto p = mesh.point({double(i) / (2 * n[0]), double(j) / (2 * n[1]),
                                                   double(k) / (2 * n[2])});
                        const double angularWeight =
                            j % 2 ? std::cos((domain[3] - domain[2]) / (2 * n[1])) : 1;
                        if (mesh.getType() == MeshType::Cylindrical) {
                            points.push_back({p[0] / angularWeight, p[1] / angularWeight, p[2]});
                            weights.push_back(angularWeight);
                        } else {
                            const double azimuthWeight =
                                k % 2 ? std::cos((domain[5] - domain[4]) / (2 * n[2])) : 1;
                            points.push_back({p[0] / (angularWeight * azimuthWeight),
                                              p[1] / (angularWeight * azimuthWeight),
                                              p[2] / angularWeight});
                            weights.push_back(angularWeight * azimuthWeight);
                        }
                    }
                }
            }
            for (std::size_t k = 0; k < n[2]; ++k) {
                for (std::size_t j = 0; j < n[1]; ++j) {
                    for (std::size_t i = 0; i < n[0]; ++i) {
                        std::array<std::size_t, 27> cell;
                        for (unsigned q = 0; q < 27; ++q) {
                            const auto& offset = bezierOffsets[q];
                            cell[q] = id(2 * i + offset[0], 2 * j + offset[1], 2 * k + offset[2]);
                        }
                        cells.push_back(cell);
                    }
                }
            }
        }
    };
} // namespace cfd::detail
