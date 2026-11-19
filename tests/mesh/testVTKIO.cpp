#include <cfd/mesh/BoundaryPatchBuilder.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>
#include <cfd/meshio/MeshGenerator.hpp>
#include <cfd/meshio/VTKReader.hpp>
#include <cfd/meshio/VTKWriter.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#include "TestUtils.hpp"

namespace {

    void compare(const cfd::Mesh& a, const cfd::Mesh& b) {
        test::require(a.getType() == b.getType() && a.getDomain() == b.getDomain(),
                      "Mesh properties round trip");
        test::require(a.nodes.getNumber() == b.nodes.getNumber(), "Node count");
        test::require(a.cells.getNumber() == b.cells.getNumber(), "Cell count");
        test::require(a.faces.getNumber() == b.faces.getNumber(), "Face count");
        for (std::size_t p = 0; p < a.nodes.getNumber(); ++p) {
            test::require(a.nodes.getCoordinates(p) == b.nodes.getCoordinates(p),
                          "Coordinates round trip");
        }
        for (std::size_t c = 0; c < a.cells.getNumber(); ++c) {
            test::require(a.cells.getNodes(c) == b.cells.getNodes(c), "Cell nodes");
            test::require(a.cells.getFaces(c) == b.cells.getFaces(c), "Cell faces");
            test::require(test::approx(a.cells.getVolume(c), b.cells.getVolume(c)), "Cell volume");
            for (unsigned d = 0; d < 3; ++d) {
                test::require(test::approx(a.cells.getCenter(c)[d], b.cells.getCenter(c)[d]),
                              "Cell center");
            }
        }
        for (std::size_t f = 0; f < a.faces.getNumber(); ++f) {
            test::require(a.faces.getNodes(f) == b.faces.getNodes(f), "Face nodes");
            test::require(a.faces.getOwner(f) == b.faces.getOwner(f), "Face owner");
            test::require(a.faces.getNeighbor(f) == b.faces.getNeighbor(f), "Face neighbor");
            test::require(a.faces.isBoundary(f) == b.faces.isBoundary(f), "Boundary detection");
            test::require(a.faces.getBoundaryPatchId(f) == b.faces.getBoundaryPatchId(f),
                          "Boundary patch ID");
            test::require(test::approx(a.faces.getArea(f), b.faces.getArea(f)), "Face area");
            for (unsigned d = 0; d < 3; ++d) {
                test::require(test::approx(a.faces.getCenter(f)[d], b.faces.getCenter(f)[d]),
                              "Face center");
                test::require(
                    test::approx(a.faces.getAreaVector(f)[d], b.faces.getAreaVector(f)[d]),
                    "Face area vector");
            }
        }
    }

    // Evaluate the exported rational geometry independently of the mesh reader.
    // Checking radius at off-grid parameter values distinguishes arcs from chords.
    void checkCurvedExport(const std::string& xml, const cfd::Mesh& mesh) {
        const auto array = [&](const std::string& marker) {
            const auto start = xml.find('>', xml.find(marker)) + 1;
            const auto end = xml.find("</DataArray>", start);
            std::istringstream stream(xml.substr(start, end - start));
            std::vector<double> values;
            double value;
            while (stream >> value) {
                values.push_back(value);
            }
            return values;
        };
        const auto coordinates = array("NumberOfComponents=\"3\" format=\"ascii\"");
        const auto weights = array("Name=\"RationalWeights\"");
        const auto connectivity = array("Name=\"connectivity\"");
        // VTK quadratic hexahedron ordering, expressed as tensor grid indices.
        constexpr std::array<std::array<unsigned, 3>, 27> indices{
            {{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}, {0, 0, 2}, {2, 0, 2}, {2, 2, 2},
             {0, 2, 2}, {1, 0, 0}, {2, 1, 0}, {1, 2, 0}, {0, 1, 0}, {1, 0, 2}, {2, 1, 2},
             {1, 2, 2}, {0, 1, 2}, {0, 0, 1}, {2, 0, 1}, {2, 2, 1}, {0, 2, 1}, {0, 1, 1},
             {2, 1, 1}, {1, 0, 1}, {1, 2, 1}, {1, 1, 0}, {1, 1, 2}, {1, 1, 1}}};
        const auto basis = [](double t) {
            return std::array<double, 3>{(1 - t) * (1 - t), 2 * t * (1 - t), t * t};
        };
        for (std::size_t cell = 0; cell < mesh.cells.getNumber(); ++cell) {
            for (double u : {0., 0.23, 1.}) {
                const auto radial = basis(u), angular = basis(0.37), third = basis(0.61);
                double denominator = 0;
                std::array<double, 3> point{};
                for (unsigned q = 0; q < 27; ++q) {
                    const auto id = static_cast<std::size_t>(connectivity.at(27 * cell + q));
                    const auto& index = indices[q];
                    const double coefficient =
                        radial[index[0]] * angular[index[1]] * third[index[2]] * weights.at(id);
                    denominator += coefficient;
                    for (unsigned axis = 0; axis < 3; ++axis) {
                        point[axis] += coefficient * coordinates.at(3 * id + axis);
                    }
                }
                for (auto& value : point) {
                    value /= denominator;
                }
                const double radius = mesh.getType() == cfd::MeshType::Cylindrical
                                          ? std::hypot(point[0], point[1])
                                          : std::hypot(point[0], point[1], point[2]);
                const auto cellIndex = mesh.cells.getIndicesFromId(cell);
                const double expected =
                    mesh.getDomain()[0] + (cellIndex[0] + u) *
                                              (mesh.getDomain()[1] - mesh.getDomain()[0]) /
                                              mesh.getResolution()[0];
                test::require(test::approx(radius, expected, 1e-12),
                              "Exported Bezier radius is exact between control points");
            }
        }
    }

    void replace(std::string& text, const std::string& before, const std::string& after) {
        const auto pos = text.find(before);
        test::require(pos != std::string::npos, "Test fixture replacement");
        text.replace(pos, before.size(), after);
    }

    void save(const std::string& path, const std::string& text) {
        std::ofstream file(path);
        file << text;
        file.close();
        test::require(bool(file), "Write fixture");
    }

    void rejects(const std::string& path) {
        try {
            (void)cfd::VTKReader::read(path);
        } catch (const std::runtime_error&) {
            return;
        }
        throw std::runtime_error("Expected rejection of invalid VTU");
    }

} // namespace

// ===================================================================================== //
//                         VTU loading and error handling tests                          //
// ===================================================================================== //
int main() {
    const std::string path = "vtk-io-test.vtu";
    try {
        const double pi = std::numbers::pi;
        for (const cfd::MeshConfig config :
             {cfd::MeshConfig{
                  cfd::MeshType::Cartesian, {-2, 3, -1, 2, 4, 8}, {4, 3, 2}, "translated"},
              cfd::MeshConfig{
                  cfd::MeshType::Cylindrical, {1, 2, -pi / 4, pi / 2, -2, 3}, {2, 4, 3}, "annulus"},
              cfd::MeshConfig{
                  cfd::MeshType::Cylindrical, {1, 2, 0, 2 * pi, 0, 3}, {2, 8, 2}, "full-ring"},
              cfd::MeshConfig{cfd::MeshType::Spherical,
                              {1, 2, pi / 6, 2 * pi / 3, 0, 2 * pi},
                              {2, 4, 8},
                              "shell"}}) {
            auto original = cfd::MeshGenerator::generate(config);
            cfd::VTKWriter::write(original, path);
            const auto loaded = cfd::VTKReader::read(path);
            cfd::MeshConnectivityBuilder::build(original);
            cfd::MeshGeometryProcessor::compute(original);
            cfd::MeshBoundaryPatchBuilder::build(original);
            compare(original, loaded);
            test::require(loaded.getResolution() == config.resolution &&
                              loaded.getName() == config.name,
                          "Name and resolution round trip");
            if (original.isCurved()) {
                std::ifstream stream(path);
                const std::string text{std::istreambuf_iterator<char>(stream), {}};
                test::require(text.find("RationalWeights=\"RationalWeights\"") !=
                                      std::string::npos &&
                                  text.find("          79\n") != std::string::npos,
                              "Curved export uses rational Bezier cells");
                checkCurvedExport(text, original);
                auto broken = text;
                replace(broken, "Name=\"RationalWeights\"", "Name=\"missing_weights\"");
                save(path, broken);
                rejects(path);
                broken = text;
                replace(broken, "          79\n", "          12\n");
                save(path, broken);
                rejects(path);
            }
        }

        // External linear hexahedron: explicit vertices, no domain metadata.
        cfd::Mesh external;
        for (const auto& p : std::array<std::array<double, 3>, 8>{{{0, 0, 0},
                                                                   {1, 0, 0},
                                                                   {1, 1, 0},
                                                                   {0, 1, 0},
                                                                   {0, 0, 1},
                                                                   {1, 0, 1},
                                                                   {1, 1, 1},
                                                                   {0, 1, 1}}}) {
            external.nodes.addCoordinates(p[0], p[1], p[2]);
        }
        external.cells.addByNodes({0, 1, 2, 3, 4, 5, 6, 7}, 0);
        cfd::VTKWriter::write(external, path);
        std::ifstream input(path);
        const std::string valid{std::istreambuf_iterator<char>(input), {}};
        input.close();
        auto variant = valid;
        replace(variant, "NumberOfPoints=\"8\" NumberOfCells=\"1\"",
                "NumberOfCells='1' NumberOfPoints='8'");
        replace(variant, "<Points>", "<!-- mesh --> <Points>");
        save(path, variant);
        auto loaded = cfd::VTKReader::read(path);
        test::require(!loaded.hasProperties() && loaded.faces.getNumber() == 6,
                      "External ASCII VTU and XML whitespace/comments");
        cfd::MeshConnectivityBuilder::build(external);
        cfd::MeshGeometryProcessor::compute(external);
        compare(external, loaded);
        const std::pair<std::string, std::string> invalid[]{
            {"format=\"ascii\"", "format=\"binary\""},
            {"          12\n", "          10\n"},
            {"          8\n", "          7\n"},
            {"0 1 2 3 4 5 6 7", "99 1 2 3 4 5 6 7"},
            {"0 1 2 3 4 5 6 7", "-1 1 2 3 4 5 6 7"},
            {"0 1 2 3 4 5 6 7", "0 0 2 3 4 5 6 7"},
            {"0 1 2 3 4 5 6 7", "0 1 2 3 4 5 6"},
            {"0 1 2 3 4 5 6 7", "0 1 2 3 4 5 6 7 8"},
            {"NumberOfPoints=\"8\"", "NumberOfPoints=\"18446744073709551616\""},
            {"</Cells>", "</Points>"},
            {"</VTKFile>", ""},
            {"</Piece>", "</Piece><Piece NumberOfPoints='0' NumberOfCells='0'/>"},
            {"<Cells>",
             "<Cells><DataArray Name='offsets' type='UInt64' format='ascii'>8</DataArray>"},
            {"          0 0 0", "          nan 0 0"}};
        for (const auto& [before, after] : invalid) {
            variant = valid;
            replace(variant, before, after);
            save(path, variant);
            rejects(path);
        }
        auto generated =
            cfd::MeshGenerator::generate({cfd::MeshType::Cartesian, {0, 1, 0, 1, 0, 1}, {2, 2, 2}});
        cfd::VTKWriter::write(generated, path);
        test::require(
            cfd::VTKReader::read(path, cfd::MeshType::Cartesian, {0, 1, 0, 1, 0, 1}, {2, 2, 2})
                    .getResolution() == cfd::MeshResolution{2, 2, 2},
            "Explicit metadata consistency");
        std::ifstream metadataInput(path);
        const std::string metadata{std::istreambuf_iterator<char>(metadataInput), {}};
        metadataInput.close();
        for (auto field :
             {"cfd_mesh_type", "cfd_mesh_domain", "cfd_mesh_resolution", "cfd_mesh_name"}) {
            variant = metadata;
            replace(variant, field, "missing_field");
            save(path, variant);
            rejects(path);
        }
        cfd::VTKWriter::write({}, path);
        test::require(cfd::VTKReader::read(path).cells.getNumber() == 0, "Empty VTU");
        std::filesystem::remove(path);
        rejects(path);
        std::cout << "All VTK IO tests passed!\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove(path);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
