#include <cfd/mesh/BoundaryPatchBuilder.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>
#include <cfd/meshio/MeshGenerator.hpp>

#include <iostream>
#include <limits>

#include "TestUtils.hpp"

namespace {

    template <class Function> void rejects(Function action) {
        try {
            action();
        } catch (const std::exception&) {
            return;
        }
        throw std::runtime_error("Expected invalid/unsupported properties to be rejected");
    }

    void checkPatches(const cfd::Mesh& mesh, std::size_t nx, std::size_t ny, std::size_t nz) {
        std::array<std::size_t, 6> counts{};
        for (std::size_t f = 0; f < mesh.faces.getNumber(); ++f) {
            const auto patch = mesh.faces.getBoundaryPatchId(f);
            if (!mesh.faces.isBoundary(f)) {
                test::require(patch == cfd::constants::INVALID_ID, "Internal face has a patch");
                continue;
            }
            test::require(patch < 6, "Unclassified boundary face");
            ++counts[patch];
            const auto axis = patch / 2;
            const auto position = mesh.getDomain()[patch];
            for (const auto node : mesh.faces.getNodes(f)) {
                test::require(test::approx(mesh.nodes.getCoordinates(node)[axis], position),
                              "Wrong patch plane");
            }
        }
        test::require(counts == std::array<std::size_t, 6>{ny * nz, ny * nz, nx * nz, nx * nz,
                                                           nx * ny, nx * ny},
                      "Boundary face counts");
    }

} // namespace

// ===================================================================================== //
//                         Metadata and boundary regression tests                        //
// ===================================================================================== //
int main() {
    try {
        using cfd::Mesh;
        using cfd::MeshType;
        Mesh empty;
        test::require(!empty.hasProperties() && empty.getDomain() == cfd::MeshDomain{} &&
                          empty.getResolution() == cfd::MeshResolution{},
                      "Default mesh");
        // Preserve the useful container checks from the retired preliminary test.
        test::require(empty.nodes.getNumber() == 0 && empty.faces.getNumber() == 0 &&
                          empty.cells.getNumber() == 0,
                      "Empty entity containers");
        {
            Mesh containers;
            containers.nodes.addCoordinates(0., 0., 0.);
            containers.nodes.addCoordinates(1., 2., 3.);
            test::require(containers.nodes.getNumber() == 2 &&
                              containers.nodes.getCoordinates(1) ==
                                  std::array<double, 3>{1., 2., 3.},
                          "Node insertion");
            rejects([&] {
                containers.nodes.getCoordinates(2);
            });
            containers.faces.addByNodes({0, 1, 2, 3}, 0, 1);
            containers.faces.addByNodes({4, 5, 6, 7}, 1);
            test::require(
                containers.faces.getNumber() == 2 && containers.faces.getOwner(0) == 0 &&
                    containers.faces.getNeighbor(0) == 1 && !containers.faces.isBoundary(0) &&
                    containers.faces.getNodes(1) == std::array<std::size_t, 4>{4, 5, 6, 7} &&
                    containers.faces.getOwner(1) == 1 && containers.faces.isBoundary(1) &&
                    containers.faces.getNeighbor(1) == cfd::constants::INVALID_ID,
                "Face insertion and adjacency");
            rejects([&] {
                containers.faces.getNodes(2);
            });
        }
        Mesh mesh(MeshType::Cartesian, {-2, 3, -1, 2, 4, 8}, {4, 3, 2}, "offset-box");
        test::require(mesh.getDomain() == cfd::MeshDomain{-2, 3, -1, 2, 4, 8} &&
                          mesh.getResolution() == cfd::MeshResolution{4, 3, 2} &&
                          mesh.getName() == "offset-box",
                      "Domain constructor");
        rejects([&] {
            mesh.setProperties(MeshType::Cartesian, {0, 0, 0, 1, 0, 1}, {2, 2, 2});
        });
        test::require(mesh.getDomain()[0] == -2, "Atomic property setter");
        for (auto resolution : {cfd::MeshResolution{0, 2, 2}, cfd::MeshResolution{2, 1, 2}}) {
            rejects([&] {
                Mesh m(MeshType::Cartesian, {0, 1, 0, 1, 0, 1}, resolution);
            });
        }
        for (double value :
             {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
            rejects([&] {
                Mesh m(MeshType::Cartesian, {0, value, 0, 1, 0, 1}, {2, 2, 2});
            });
        }
        rejects([] {
            Mesh m(static_cast<MeshType>(99), {}, {});
        });
        rejects([] {
            Mesh m(MeshType::Cylindrical, {0, 2, 0, 1, 0, 1}, {2, 2, 2});
        });
        rejects([] {
            Mesh m(MeshType::Spherical, {1, 2, 0, 1, 0, 1}, {2, 2, 2});
        });
        auto offset =
            cfd::MeshGenerator::generate({MeshType::Cartesian, {-2, 3, -1, 2, 4, 8}, {4, 3, 2}});
        cfd::MeshConnectivityBuilder::build(offset);
        cfd::MeshGeometryProcessor::compute(offset);
        cfd::MeshBoundaryPatchBuilder::buildCartesian(offset);
        checkPatches(offset, 4, 3, 2);
        test::require(test::approx(offset.cells.getCenter(0)[0], -1.375), "Translated cell center");

        for (double scale : {1e-5, 1.0, 1e5}) {
            auto cartesian = cfd::MeshGenerator::generate(
                {cfd::MeshType::Cartesian, {0, 2 * scale, 0, 3 * scale, 0, 4 * scale}, {4, 3, 2}});
            cfd::MeshConnectivityBuilder::build(cartesian);
            cfd::MeshGeometryProcessor::compute(cartesian);
            cfd::MeshBoundaryPatchBuilder::buildCartesian(cartesian);
            checkPatches(cartesian, 4, 3, 2);
            cfd::MeshBoundaryPatchBuilder::buildCartesian(cartesian); // Safe to repeat.
            checkPatches(cartesian, 4, 3, 2);
            cartesian.setProperties(MeshType::Cartesian, {0, 5 * scale, 0, 3 * scale, 0, 4 * scale},
                                    {4, 3, 2});
            rejects([&] {
                cfd::MeshBoundaryPatchBuilder::buildCartesian(cartesian);
            });
        }
        std::cout << "All mesh property and boundary tests passed!\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
