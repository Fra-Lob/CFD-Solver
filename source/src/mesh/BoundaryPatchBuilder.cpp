#include <cfd/mesh/BoundaryPatchBuilder.hpp>

#include <algorithm>
#include <limits>

namespace cfd {

    // ===================================================================================== //
    //                           Boundary classification dispatch                            //
    // ===================================================================================== //
    void MeshBoundaryPatchBuilder::build(Mesh& mesh) {
        if (mesh.getType() == MeshType::Cartesian) {
            buildCartesian(mesh);
        } else if (mesh.getType() == MeshType::Cylindrical ||
                   mesh.getType() == MeshType::Spherical) {
            buildLogical(mesh);
        } else {
            throw std::invalid_argument("Custom patches must come from physical surface groups");
        }
    }

    // ===================================================================================== //
    //                               Logical sector boundaries                               //
    // ===================================================================================== //
    void MeshBoundaryPatchBuilder::buildLogical(Mesh& mesh) {

        if (mesh.getType() != MeshType::Cylindrical && mesh.getType() != MeshType::Spherical) {
            throw std::invalid_argument("Logical patch classification requires a generated sector");
        }
        const std::array<std::size_t, 3> size = {mesh.nodes.getNx(), mesh.nodes.getNy(),
                                                 mesh.nodes.getNz()};
        for (auto n : size) {
            if (n < 2) {
                throw std::invalid_argument("Missing logical node dimensions");
            }
        }
        const auto names =
            mesh.getType() == MeshType::Cylindrical
                ? std::array<std::string, 6>{"inner",       "outer",       "angular_start",
                                             "angular_end", "axial_start", "axial_end"}
                : std::array<std::string, 6>{"inner",     "outer",         "polar_start",
                                             "polar_end", "azimuth_start", "azimuth_end"};
        std::map<std::size_t, std::string> metadata;
        for (unsigned p = 0; p < 6; ++p) {
            if (!mesh.isPeriodic(p / 2)) {
                metadata.emplace(p, names[p]);
            }
        }
        std::vector<std::size_t> patches(mesh.faces.getNumber(), constants::INVALID_ID);
        for (std::size_t f = 0; f < mesh.faces.getNumber(); ++f) {
            if (mesh.faces.isBoundary(f)) {
                for (unsigned axis = 0; axis < 3; ++axis) {
                    if (mesh.isPeriodic(axis)) {
                        continue;
                    }
                    for (unsigned side = 0; side < 2; ++side) {
                        const auto nodes = mesh.faces.getNodes(f);
                        if (std::all_of(nodes.begin(), nodes.end(), [&](auto n) {
                                return mesh.nodes.getIndicesFromId(n)[axis] ==
                                       (side == 0 ? 0 : size[axis] - 1);
                            })) {
                            if (patches[f] != constants::INVALID_ID) {
                                throw std::runtime_error("Ambiguous logical patch at face " +
                                                         std::to_string(f));
                            }
                            patches[f] = 2 * axis + side;
                        }
                    }
                }
                if (patches[f] == constants::INVALID_ID) {
                    throw std::runtime_error("Missing logical patch at face " + std::to_string(f));
                }
            }
        }
        for (std::size_t f = 0; f < patches.size(); ++f) {
            mesh.faces.setBoundaryPatchId(f, patches[f]);
        }
        mesh.setPatchNames(metadata);
    }

    // ===================================================================================== //
    //                              Cartesian domain boundaries                              //
    // ===================================================================================== //
    void MeshBoundaryPatchBuilder::buildCartesian(Mesh& mesh) {
        mesh.requireCartesian3D();
        const auto& domain = mesh.getDomain();
        std::vector<std::size_t> patches(mesh.faces.getNumber(), constants::INVALID_ID);

        size_t nFaces = mesh.faces.getNumber();
        for (std::size_t faceId = 0; faceId < nFaces; ++faceId) {
            if (mesh.faces.isBoundary(faceId)) {
                for (unsigned axis = 0; axis < 3; ++axis) {
                    const auto tolerance =
                        64 * std::numeric_limits<double>::epsilon() *
                        std::max({std::abs(domain[2 * axis]), std::abs(domain[2 * axis + 1]),
                                  domain[2 * axis + 1] - domain[2 * axis]});
                    for (unsigned side = 0; side < 2; ++side) {
                        const double coordinate = domain[2 * axis + side];
                        const auto nodes = mesh.faces.getNodes(faceId);
                        const bool onPlane =
                            std::all_of(nodes.begin(), nodes.end(), [&](std::size_t node) {
                                return std::abs(mesh.nodes.getCoordinates(node)[axis] -
                                                coordinate) <= tolerance;
                            });
                        if (onPlane) {
                            if (patches[faceId] != constants::INVALID_ID) {
                                throw std::runtime_error(
                                    "Boundary face matches multiple domain planes");
                            }
                            patches[faceId] = 2 * axis + side; // xmin/xmax/ymin/ymax/zmin/zmax
                        }
                    }
                }
                if (patches[faceId] == constants::INVALID_ID) {
                    throw std::runtime_error(
                        "Boundary face does not lie on the declared Cartesian domain");
                }
            }
        }
        // Commit only after every boundary face has been classified successfully.
        for (std::size_t faceId = 0; faceId < nFaces; ++faceId) {
            mesh.faces.setBoundaryPatchId(faceId, patches[faceId]);
        }
        mesh.setPatchNames(
            {{0, "xmin"}, {1, "xmax"}, {2, "ymin"}, {3, "ymax"}, {4, "zmin"}, {5, "zmax"}});
    }
} // namespace cfd
