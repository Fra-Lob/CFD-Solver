#include "TestUtils.hpp"

#include <cfd/mesh/BoundaryPatchBuilder.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>
#include <cfd/meshio/MeshGenerator.hpp>

#include <iostream>
#include <limits>

namespace {

template<class Function>
void rejects(Function action)
{
    try { action(); }
    catch (const std::exception&) { return; }
    throw std::runtime_error("Expected invalid/unsupported properties to be rejected");
}

void checkPatches(const cfd::Mesh& mesh, std::size_t nx, std::size_t ny, std::size_t nz)
{
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
        const auto position = (patch % 2 == 0) ? 0.0 : mesh.getDomain()[axis];
        for (const auto node : mesh.faces.getNodes(f))
            test::require(test::approx(mesh.nodes.getCoordinates(node)[axis], position), "Wrong patch plane");
    }
    test::require(counts == std::array<std::size_t, 6>{ny*nz, ny*nz, nx*nz, nx*nz, nx*ny, nx*ny},
                  "Boundary face counts");
}

} // namespace

int main()
{
    try {
        using cfd::Mesh;
        using cfd::MeshType;
        Mesh empty;
        test::require(!empty.hasProperties() && empty.getDimension() == 0 && empty.getDomain().empty(), "Default mesh");
        for (auto type : {MeshType::Cartesian, MeshType::Cylindrical, MeshType::Spherical}) {
            for (unsigned d = 1; d <= 3; ++d) {
                std::vector<double> domain(d, 1.0);
                Mesh mesh(type, d, domain);
                test::require(mesh.getType() == type && mesh.getDimension() == d && mesh.getDomain() == domain,
                              "Constructor properties");
            }
        }
        Mesh mesh(MeshType::Cartesian, 3, {2., 3., 4.});
        rejects([&] { mesh.setProperties(MeshType::Cartesian, 3, {1., 2.}); });
        test::require(mesh.getDomain() == std::vector<double>{2., 3., 4.}, "Failed setter modified properties");
        mesh.setProperties(MeshType::Cylindrical, 2, {5., 2.});
        test::require(mesh.getType() == MeshType::Cylindrical && mesh.getDimension() == 2 &&
                      mesh.getDomain() == std::vector<double>{5., 2.}, "Setter properties");

        rejects([] { Mesh m(MeshType::Unknown, 3, {1., 1., 1.}); });
        rejects([] { Mesh m(static_cast<MeshType>(99), 3, {1., 1., 1.}); });
        rejects([] { Mesh m(MeshType::Cartesian, 0, {}); });
        rejects([] { Mesh m(MeshType::Cartesian, 4, {1., 1., 1., 1.}); });
        for (double value : {0.0, -1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
            rejects([&] { Mesh m(MeshType::Cartesian, 1, {value}); });
        rejects([] { Mesh m(MeshType::Cylindrical, 3, {1., 1., 7.}); });
        rejects([] { Mesh m(MeshType::Spherical, 2, {1., 4.}); });
        rejects([] { Mesh m(MeshType::Spherical, 3, {1., 1., 7.}); });
        rejects([] { cfd::MeshGenerator::generate({MeshType::Cylindrical, 3, {1., 1., 1.}, 1, 1, 1}); });
        rejects([] { cfd::MeshGenerator::generate({MeshType::Cartesian, 2, {1., 1.}, 1, 1, 1}); });
        rejects([] { cfd::MeshGenerator::generate({MeshType::Cartesian, 3, {1., 1., 1.}, 0, 1, 1}); });
        rejects([&] { cfd::MeshConnectivityBuilder::build(mesh); });
        rejects([&] { cfd::MeshGeometryProcessor::compute(mesh); });
        rejects([&] { cfd::MeshBoundaryPatchBuilder::buildCartesian(mesh); });

        for (double scale : {1e-5, 1.0, 1e5}) {
            auto cartesian = cfd::MeshGenerator::generate({MeshType::Cartesian, 3, {2*scale, 3*scale, 4*scale}, 4, 3, 2});
            cfd::MeshConnectivityBuilder::build(cartesian);
            cfd::MeshGeometryProcessor::compute(cartesian);
            cfd::MeshBoundaryPatchBuilder::buildCartesian(cartesian);
            checkPatches(cartesian, 4, 3, 2);
            cfd::MeshBoundaryPatchBuilder::buildCartesian(cartesian); // Safe to repeat.
            checkPatches(cartesian, 4, 3, 2);
            cartesian.setProperties(MeshType::Cartesian, 3, {5*scale, 3*scale, 4*scale});
            rejects([&] { cfd::MeshBoundaryPatchBuilder::buildCartesian(cartesian); });
        }
        std::cout << "All mesh property and boundary tests passed!\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
