#include <cfd/meshio/MeshGenerator.hpp>

#include <limits>
#include <numbers>

namespace cfd {
    namespace {
        // ===================================================================================== //
        //                              Resolution and grid helpers                              //
        // ===================================================================================== //
        std::size_t checkedProduct(std::size_t a, std::size_t b) {
            if (b && a > std::numeric_limits<std::size_t>::max() / b) {
                throw std::invalid_argument("Mesh resolution overflows entity counts");
            }
            return a * b;
        }

        Mesh generateGrid(const MeshConfig& config) {
            Mesh mesh(config.type, config.domain, config.resolution, config.name);
            mesh.cells.setDimensions(config.resolution[0], config.resolution[1],
                                     config.resolution[2]);
            const auto size = mesh.nodeResolution();
            mesh.nodes.setDimensions(size[0], size[1], size[2]);
            for (std::size_t k = 0; k < size[2]; ++k) {
                for (std::size_t j = 0; j < size[1]; ++j) {
                    for (std::size_t i = 0; i < size[0]; ++i) {
                        const auto p = mesh.point({double(i) / config.resolution[0],
                                                   double(j) / config.resolution[1],
                                                   double(k) / config.resolution[2]});
                        mesh.nodes.addCoordinates(p[0], p[1], p[2]);
                    }
                }
            }
            for (std::size_t k = 0; k < config.resolution[2]; ++k) {
                for (std::size_t j = 0; j < config.resolution[1]; ++j) {
                    for (std::size_t i = 0; i < config.resolution[0]; ++i) {
                        const auto n = [&](auto x, auto y, auto z) {
                            return mesh.nodes.getIdFromIndices(x % size[0], y % size[1],
                                                               z % size[2]);
                        };
                        mesh.cells.addByNodes({n(i, j, k), n(i + 1, j, k), n(i + 1, j + 1, k),
                                               n(i, j + 1, k), n(i, j, k + 1), n(i + 1, j, k + 1),
                                               n(i + 1, j + 1, k + 1), n(i, j + 1, k + 1)},
                                              mesh.cells.getNumber());
                    }
                }
            }
            return mesh;
        }
    } // namespace

    // ===================================================================================== //
    //                                   Input validation                                    //
    // ===================================================================================== //
    void MeshGenerator::validate(const MeshConfig& config) {
        Mesh descriptor(config.type, config.domain, config.resolution, config.name);
        if (config.type == MeshType::Custom || config.type == MeshType::Unknown) {
            throw std::invalid_argument("MeshGenerator requires an internal geometry type");
        }
        checkedProduct(checkedProduct(config.resolution[0] + 1, config.resolution[1] + 1),
                       config.resolution[2] + 1);
        checkedProduct(checkedProduct(config.resolution[0], config.resolution[1]),
                       config.resolution[2]);
    }

    // ===================================================================================== //
    //                                  Generation dispatch                                  //
    // ===================================================================================== //
    Mesh MeshGenerator::generate(const MeshConfig& config) {
        validate(config);
        return generateGrid(config);
    }
} // namespace cfd
