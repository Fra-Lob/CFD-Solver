#pragma once

#include <cfd/mesh/Mesh.hpp>

#include <algorithm>
#include <limits>

namespace cfd::detail {

    // ===================================================================================== //
    //                              Hexahedral reference topology                            //
    // ===================================================================================== //

    inline constexpr std::array<std::array<unsigned, 4>, 6> localFaces = {
        {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4}, {3, 7, 6, 2}, {0, 4, 7, 3}, {1, 2, 6, 5}}};

    inline std::array<std::size_t, 4> canonicalFace(std::array<std::size_t, 4> nodes) {
        // Both cells sharing a face must use the same triangulation diagonal.
        std::rotate(nodes.begin(), std::min_element(nodes.begin(), nodes.end()), nodes.end());
        return nodes;
    }

    inline math::Vector3 subtract(const math::Vector3& firstVector,
                                  const math::Vector3& secondVector) {
        return math::vectorSum(firstVector, math::opposite(secondVector));
    }

    // ===================================================================================== //
    //                             Jacobian positivity certification                         //
    // ===================================================================================== //

    using JacobianCoefficients = std::array<double, 27>;

    inline bool positiveBernstein(const JacobianCoefficients& coefficients,
                                  unsigned subdivisionDepth,
                                  double tolerance) {
        const auto [minimum, maximum] =
            std::minmax_element(coefficients.begin(), coefficients.end());

        if (*minimum > tolerance) {
            return true;
        }

        // Reject negative bounds and unresolved nearly singular subdomains.
        if (*maximum <= tolerance || subdivisionDepth == 18) {
            return false;
        }

        const unsigned axisStride =
            subdivisionDepth % 3 == 0 ? 1 : (subdivisionDepth % 3 == 1 ? 3 : 9);

        JacobianCoefficients leftCoefficients = coefficients;
        JacobianCoefficients rightCoefficients = coefficients;

        // De Casteljau subdivision at the midpoint of the selected reference axis.
        for (unsigned index = 0; index < coefficients.size(); ++index) {
            if ((index / axisStride) % 3 != 0) {
                continue;
            }

            const double first = coefficients[index];
            const double middle = coefficients[index + axisStride];
            const double last = coefficients[index + 2 * axisStride];
            const double midpoint = (first + 2 * middle + last) / 4;

            leftCoefficients[index] = first;
            leftCoefficients[index + axisStride] = (first + middle) / 2;
            leftCoefficients[index + 2 * axisStride] = midpoint;

            rightCoefficients[index] = midpoint;
            rightCoefficients[index + axisStride] = (middle + last) / 2;
            rightCoefficients[index + 2 * axisStride] = last;
        }

        return positiveBernstein(leftCoefficients, subdivisionDepth + 1, tolerance) &&
               positiveBernstein(rightCoefficients, subdivisionDepth + 1, tolerance);
    }

    // Certify the trilinear hex Jacobian over the complete reference cube.
    // Ambiguous, nearly singular and inverted elements are rejected conservatively.
    inline void checkHex(const Mesh& mesh, std::size_t cellId) {
        const auto cellNodes = mesh.cells.getNodes(cellId);
        const auto origin = mesh.nodes.getCoordinates(cellNodes[0]);
        std::array<math::Vector3, 8> points;
        double scale = 0;

        const auto rejectCell = [&] {
            throw std::runtime_error("Invalid, inverted or uncertifiable hexahedron at cell " +
                                     std::to_string(cellId));
        };

        // Normalize coordinates to make the determinant test independent of units.
        for (unsigned node = 0; node < cellNodes.size(); ++node) {
            points[node] = subtract(mesh.nodes.getCoordinates(cellNodes[node]), origin);

            for (double component : points[node]) {
                scale = std::max(scale, std::abs(component));
            }
        }

        if (!std::isfinite(scale) || scale <= 0) {
            rejectCell();
        }

        for (auto& point : points) {
            for (double& component : point) {
                component /= scale;
            }
        }

        constexpr unsigned referenceCorners[8][3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                                     {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
        JacobianCoefficients coefficients{};

        // Evaluate the determinant at a 3 x 3 x 3 reference grid.
        for (unsigned sampleZ = 0; sampleZ < 3; ++sampleZ) {
            for (unsigned sampleY = 0; sampleY < 3; ++sampleY) {
                for (unsigned sampleX = 0; sampleX < 3; ++sampleX) {
                    const double referencePoint[3] = {sampleX / 2., sampleY / 2., sampleZ / 2.};
                    std::array<math::Vector3, 3> jacobian{};

                    for (unsigned node = 0; node < 8; ++node) {
                        for (unsigned direction = 0; direction < 3; ++direction) {
                            double weight = referenceCorners[node][direction] ? 1. : -1.;

                            for (unsigned axis = 0; axis < 3; ++axis) {
                                if (axis != direction) {
                                    weight *= referenceCorners[node][axis]
                                                  ? referencePoint[axis]
                                                  : 1 - referencePoint[axis];
                                }
                            }

                            jacobian[direction] =
                                math::vectorSum(jacobian[direction],
                                                math::productByCoefficient(points[node], weight));
                        }
                    }

                    const unsigned index = sampleX + 3 * sampleY + 9 * sampleZ;
                    coefficients[index] = math::scalarProduct(
                        jacobian[0], math::vectorProduct(jacobian[1], jacobian[2]));
                }
            }
        }

        // Convert the samples at 0, 1/2 and 1 to tensor-product Bernstein coefficients.
        for (unsigned stride : {1u, 3u, 9u}) {
            for (unsigned index = 0; index < coefficients.size(); ++index) {
                if ((index / stride) % 3 == 0) {
                    coefficients[index + stride] =
                        2 * coefficients[index + stride] -
                        (coefficients[index] + coefficients[index + 2 * stride]) / 2;
                }
            }
        }

        double magnitude = 0;
        for (double coefficient : coefficients) {
            if (!std::isfinite(coefficient)) {
                rejectCell();
            }
            magnitude = std::max(magnitude, std::abs(coefficient));
        }

        const double tolerance = 256 * std::numeric_limits<double>::epsilon() * magnitude;
        if (!positiveBernstein(coefficients, 0, tolerance)) {
            rejectCell();
        }
    }
} // namespace cfd::detail
