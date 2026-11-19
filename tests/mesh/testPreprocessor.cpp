#include <cfd/mesh/BoundaryPatchBuilder.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>
#include <cfd/mesh/MeshValidator.hpp>
#include <cfd/meshio/GmshImporter.hpp>
#include <cfd/meshio/GridConfigReader.hpp>
#include <cfd/meshio/MeshReader.hpp>
#include <cfd/meshio/MeshWriter.hpp>
#include <cfd/meshio/Preprocessor.hpp>
#include <cfd/meshio/VTKReader.hpp>
#include <cfd/meshio/VTKWriter.hpp>

#include <fstream>
#include <iostream>
#include <iterator>

#include "TestUtils.hpp"

namespace fs = std::filesystem;
using namespace cfd;

namespace {
    template <class F> void rejects(F action, const std::string& label) {
        try {
            action();
        } catch (const std::exception&) {
            return;
        }
        throw std::runtime_error("Expected rejection: " + label);
    }

    void save(const fs::path& path, const std::string& text) {
        std::ofstream out(path, std::ios::binary);
        out << text;
        out.close();
        test::require(bool(out), "Cannot save test fixture");
    }

    std::string contents(const fs::path& path) {
        std::ifstream in(path, std::ios::binary);
        test::require(bool(in), "Cannot read test fixture");
        return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    }

    Mesh complete(const MeshConfig& config) {
        auto mesh = MeshGenerator::generate(config);
        MeshConnectivityBuilder::build(mesh);
        MeshGeometryProcessor::compute(mesh);
        MeshBoundaryPatchBuilder::build(mesh);
        MeshValidator::validate(mesh);
        return mesh;
    }

    double volume(const Mesh& mesh) {
        double sum = 0;
        for (std::size_t cellId = 0; cellId < mesh.cells.getNumber(); ++cellId) {
            sum += mesh.cells.getVolume(cellId);
        }
        return sum;
    }

    void counts(const Mesh& mesh, std::size_t nx, std::size_t ny, std::size_t nz) {
        test::require(mesh.nodes.getNumber() == (nx + 1) * (ny + 1) * (nz + 1), "Node count");
        test::require(mesh.cells.getNumber() == nx * ny * nz, "Cell count");
        test::require(mesh.faces.getNumber() ==
                          (nx + 1) * ny * nz + nx * (ny + 1) * nz + nx * ny * (nz + 1),
                      "Face count");
        std::array<std::size_t, 6> patches{};
        for (std::size_t f = 0; f < mesh.faces.getNumber(); ++f) {
            if (mesh.faces.isBoundary(f)) {
                test::require(mesh.faces.getBoundaryPatchId(f) < 6, "Patch ID");
                ++patches[mesh.faces.getBoundaryPatchId(f)];
            }
        }
        test::require(patches == std::array<std::size_t, 6>{ny * nz, ny * nz, nx * nz, nx * nz,
                                                            nx * ny, nx * ny},
                      "Patch counts");
    }

    void equal(const Mesh& a, const Mesh& b) {
        test::require(a.getName() == b.getName() && a.getType() == b.getType() &&
                          a.getResolution() == b.getResolution() &&
                          a.getDomain() == b.getDomain() && a.getPatchNames() == b.getPatchNames(),
                      "All domain/patch metadata");
        test::require(a.nodes.getNumber() == b.nodes.getNumber() &&
                          a.cells.getNumber() == b.cells.getNumber() &&
                          a.faces.getNumber() == b.faces.getNumber(),
                      "All entity counts");
        test::require(a.nodes.getNx() == b.nodes.getNx() && a.nodes.getNy() == b.nodes.getNy() &&
                          a.nodes.getNz() == b.nodes.getNz() &&
                          a.cells.getNx() == b.cells.getNx() &&
                          a.cells.getNy() == b.cells.getNy() && a.cells.getNz() == b.cells.getNz(),
                      "Logical dimensions");
        for (std::size_t n = 0; n < a.nodes.getNumber(); ++n) {
            test::require(a.nodes.getCoordinates(n) == b.nodes.getCoordinates(n),
                          "Node coordinates");
        }
        for (std::size_t cellId = 0; cellId < a.cells.getNumber(); ++cellId) {
            test::require(a.cells.getNodes(cellId) == b.cells.getNodes(cellId) &&
                              a.cells.getFaces(cellId) == b.cells.getFaces(cellId) &&
                              a.cells.getVolume(cellId) == b.cells.getVolume(cellId) &&
                              a.cells.getCenter(cellId) == b.cells.getCenter(cellId),
                          "All cell attributes");
        }
        for (std::size_t f = 0; f < a.faces.getNumber(); ++f) {
            test::require(a.faces.getNodes(f) == b.faces.getNodes(f) &&
                              a.faces.getOwner(f) == b.faces.getOwner(f) &&
                              a.faces.getNeighbor(f) == b.faces.getNeighbor(f) &&
                              a.faces.getCenter(f) == b.faces.getCenter(f) &&
                              a.faces.getArea(f) == b.faces.getArea(f) &&
                              a.faces.getAreaVector(f) == b.faces.getAreaVector(f) &&
                              a.faces.getBoundaryPatchId(f) == b.faces.getBoundaryPatchId(f),
                          "All face attributes");
        }
    }

    // ===================================================================================== //
    //                             Configuration regression tests                            //
    // ===================================================================================== //
    void configTests(const fs::path& temp) {
        const std::string valid = "\n# comment\nname = box\ntype=cartesian\nxMin=0\nxMax=2 # "
                                  "metres\nyMin=0\nyMax=3\nzMin=0\nzMax=4\nNx=2\nNy=3\nNz=4\n";
        save(temp / "grid.info", valid);
        auto config = GridConfigReader::read(temp);
        test::require(config.type == MeshType::Cartesian &&
                          config.domain == cfd::MeshDomain{0, 2, 0, 3, 0, 4} &&
                          config.resolution[1] == 3 && config.name == "box",
                      "Cartesian parsing");
        for (const auto& suffix : {"Nx=3\n", "junk=1\n", "bad line\n"}) {
            save(temp / "grid.info", valid + suffix);
            rejects(
                [&] {
                    GridConfigReader::read(temp);
                },
                "invalid syntax/duplicate/unknown");
        }
        for (const auto& [before, after] :
             std::initializer_list<std::pair<std::string, std::string>>{
                 {"Nx=2", "Nx=-1"},
                 {"Nx=2", "Nx=1.5"},
                 {"Nx=2", "Nx=0"},
                 {"Nx=2", "Nx=18446744073709551616"},
                 {"xMax=2", "xMax=nan"},
                 {"xMax=2", "xMax=inf"},
                 {"xMax=2", "xMax=-2"},
                 {"xMax=2", "xMax=2x"},
                 {"name = box", "name = ../escape"},
                 {"type=cartesian", "type=other"},
                 {"Ny=3\n", ""}}) {
            auto text = valid;
            text.replace(text.find(before), before.size(), after);
            save(temp / "grid.info", text);
            rejects(
                [&] {
                    GridConfigReader::read(temp);
                },
                before);
        }
        for (const auto& name : {"cylindrical", "spherical", "custom"}) {
            save(temp / "grid.info",
                 contents(fs::path(CFD_SOURCE_DIR) / "examples" / name / "grid.info"));
            config = GridConfigReader::read(temp);
            test::require(config.domain.size() == 6, "Physical dimension remains 3D");
            if (config.type == MeshType::Cylindrical) {
                test::require(
                    test::approx(config.domain[3] - config.domain[2], 2 * std::numbers::pi),
                    "Degrees to radians");
            }
        }
        fs::remove(temp / "grid.info");
        rejects(
            [&] {
                GridConfigReader::read(temp);
            },
            "missing grid.info");
        std::cout << "PASS configuration parsing and validation\n";
    }

    // ===================================================================================== //
    //                        Generated geometry and validation tests                        //
    // ===================================================================================== //
    void generatedTests() {
        auto box = complete({cfd::MeshType::Cartesian, {0, 2, 0, 3, 0, 4}, {4, 3, 2}});
        counts(box, 4, 3, 2);
        test::require(test::approx(volume(box), 24), "Cartesian analytical volume");
        const double pi = std::numbers::pi;
        for (std::size_t radialCount : {2u, 3u}) {
            for (std::size_t angularCount : {2u, 12u}) {
                auto cylinder = complete({cfd::MeshType::Cylindrical,
                                          {1, 2, -pi / 6, pi / 3, 0, 3},
                                          {radialCount, angularCount, 2}});
                counts(cylinder, radialCount, angularCount, 2);
                const double expected = 0.5 * 3 * 3 * (pi / 2);
                test::require(test::approx(volume(cylinder), expected),
                              "Exact curved cylindrical volume");
            }
        }
        const auto coarse =
            complete({cfd::MeshType::Spherical, {1, 2, pi / 6, 2 * pi / 3, 0, pi / 2}, {2, 4, 4}});
        const auto fine = complete(
            {cfd::MeshType::Spherical, {1, 2, pi / 6, 2 * pi / 3, 0, pi / 2}, {2, 16, 16}});
        counts(fine, 2, 16, 16);
        const double analytic = 7. / 3 * (std::cos(pi / 6) - std::cos(2 * pi / 3)) * (pi / 2);
        test::require(test::approx(volume(fine), analytic) &&
                          test::approx(volume(coarse), analytic),
                      "Exact spherical volume independent of resolution");
        complete({cfd::MeshType::Spherical, {1, 2, pi / 4, pi / 2, 0, pi / 3}, {2, 2, 2}});
        for (const auto& config : std::initializer_list<MeshConfig>{
                 {cfd::MeshType::Cylindrical, {0, 2, 0, pi / 2, 0, 3}, {2, 2, 2}},
                 {cfd::MeshType::Cylindrical, {1, 2, 0, 3 * pi, 0, 3}, {2, 4, 2}},
                 {cfd::MeshType::Cylindrical, {1, 2, 0, 1.5 * pi, 0, 3}, {2, 1, 2}},
                 {cfd::MeshType::Spherical, {0, 2, pi / 4, pi / 2, 0, pi / 2}, {2, 2, 2}},
                 {cfd::MeshType::Spherical, {1, 2, 0, pi / 2, 0, pi / 2}, {2, 2, 2}},
                 {cfd::MeshType::Spherical, {1, 2, pi / 4, pi, 0, pi / 2}, {2, 2, 2}},
                 {cfd::MeshType::Spherical, {1, 2, pi / 4, pi / 2, 0, 3 * pi}, {2, 2, 4}}}) {
            rejects(
                [&] {
                    MeshGenerator::generate(config);
                },
                "singular/periodic/inverted configuration");
        }
        // Independent whole-domain surface integrals and volume moments.
        const auto cylinder =
            complete({MeshType::Cylindrical, {1, 2, 0, pi / 2, -2, 3}, {2, 4, 3}});
        std::array<double, 6> areas{};
        math::Vector3 moment{};
        for (std::size_t face = 0; face < cylinder.faces.getNumber(); ++face) {
            if (cylinder.faces.isBoundary(face)) {
                areas[cylinder.faces.getBoundaryPatchId(face)] += cylinder.faces.getArea(face);
            }
        }
        const std::array<double, 6> expectedAreas{5 * pi / 2, 5 * pi, 5, 5, 3 * pi / 4, 3 * pi / 4};
        for (unsigned patch = 0; patch < 6; ++patch) {
            test::require(test::approx(areas[patch], expectedAreas[patch]),
                          "Exact cylindrical surface area");
        }
        for (std::size_t cell = 0; cell < cylinder.cells.getNumber(); ++cell) {
            moment =
                math::vectorSum(moment, math::productByCoefficient(cylinder.cells.getCenter(cell),
                                                                   cylinder.cells.getVolume(cell)));
        }
        const auto center = math::divisionByCoefficient(moment, volume(cylinder));
        test::require(test::approx(center[0], 28 / (9 * pi)) &&
                          test::approx(center[1], 28 / (9 * pi)) && test::approx(center[2], 0.5),
                      "Exact cylindrical volume centroid");
        for (auto type : {MeshType::Cylindrical, MeshType::Spherical}) {
            const MeshConfig config =
                type == MeshType::Cylindrical
                    ? MeshConfig{type, {1, 2, 0, 2 * pi, -2, 3}, {2, 8, 3}}
                    : MeshConfig{type, {1, 2, pi / 4, 3 * pi / 4, 0, 2 * pi}, {2, 4, 8}};
            const auto periodic = complete(config);
            test::require(periodic.getPatchNames().size() == 4,
                          "Full turn has four physical patches");
            const unsigned axis = type == MeshType::Cylindrical ? 1 : 2;
            test::require(periodic.nodeResolution()[axis] == periodic.getResolution()[axis],
                          "Shared seam nodes");
            double outerArea = 0;
            for (std::size_t face = 0; face < periodic.faces.getNumber(); ++face) {
                test::require(!periodic.faces.isBoundary(face) ||
                                  periodic.faces.getBoundaryPatchId(face) / 2 != axis,
                              "Periodic seam is not a boundary");
                if (periodic.faces.getBoundaryPatchId(face) == 1) {
                    outerArea += periodic.faces.getArea(face);
                }
            }
            const double expectedVolume =
                type == MeshType::Cylindrical ? 15 * pi : 14 * pi * std::sqrt(2.) / 3;
            const double expectedArea =
                type == MeshType::Cylindrical ? 20 * pi : 8 * pi * std::sqrt(2.);
            test::require(test::approx(volume(periodic), expectedVolume) &&
                              test::approx(outerArea, expectedArea),
                          "Full-turn analytic volume and outer surface");
        }
        auto corrupt = box;
        corrupt.faces.setAreaVector(0,
                                    math::productByCoefficient(corrupt.faces.getAreaVector(0), -1));
        rejects(
            [&] {
                MeshValidator::validate(corrupt);
            },
            "inward normal");
        corrupt = box;
        corrupt.faces.setBoundaryPatchId(0, constants::INVALID_ID);
        rejects(
            [&] {
                MeshValidator::validate(corrupt);
            },
            "missing patch");
        corrupt = box;
        corrupt.faces.setAreaVector(
            0, math::productByCoefficient(corrupt.faces.getAreaVector(0), 0.9));
        rejects(
            [&] {
                MeshValidator::validate(corrupt);
            },
            "closure");
        corrupt = box;
        auto nodes = corrupt.cells.getNodes(0);
        std::swap(nodes[0], nodes[1]);
        corrupt.cells.setNodes(0, nodes);
        rejects(
            [&] {
                MeshValidator::validate(corrupt);
            },
            "inverted cell");
        corrupt = box;
        corrupt.nodes.setCoordinates(0, corrupt.nodes.getCoordinates(1));
        rejects(
            [&] {
                MeshValidator::validate(corrupt);
            },
            "collapsed cell");
        corrupt = box;
        corrupt.faces.addByNodes(corrupt.faces.getNodes(0), 0);
        rejects(
            [&] {
                MeshValidator::validate(corrupt);
            },
            "duplicate face");
        std::cout << "PASS internal generators, analytical comparisons, topology, patches, "
                     "orientations, closure and invalid cells\n";
    }

    // ===================================================================================== //
    //                         Native format and VTU round-trip tests                        //
    // ===================================================================================== //
    void nativeTests(const fs::path& temp) {
        const double pi = std::numbers::pi;
        for (const auto& config : std::initializer_list<MeshConfig>{
                 {cfd::MeshType::Cartesian, {0, 2, 0, 3, 0, 4}, {3, 2, 2}, "box"},
                 {cfd::MeshType::Cylindrical, {1, 2, -pi / 6, pi / 3, 0, 3}, {2, 5, 3}, "annulus"},
                 {cfd::MeshType::Spherical,
                  {1, 2, pi / 6, pi / 2, 0, pi / 2},
                  {2, 4, 5},
                  "shell"}}) {
            const auto mesh = complete(config);
            const auto native = temp / (config.name + ".mesh"), vtu = temp / (config.name + ".vtu");
            MeshWriter::write(mesh, native);
            equal(mesh, MeshReader::read(native));
            MeshWriter::write(MeshReader::read(native), temp / "second.mesh");
            test::require(contents(native) == contents(temp / "second.mesh"),
                          "Bit-exact native round trip");
            VTKWriter::write(mesh, vtu.string());
            const auto visual = VTKReader::read(vtu.string());
            MeshValidator::validate(visual);
            test::require(test::approx(volume(visual), volume(mesh)), "VTU mesh metrics");
            auto bytes = contents(native);
            for (unsigned mutation = 0; mutation < 5; ++mutation) {
                auto broken = bytes;
                if (mutation == 0) {
                    broken[0] = 'X';
                }
                if (mutation == 1) {
                    broken[8] = 99;
                }
                if (mutation == 2) {
                    broken.resize(broken.size() / 2);
                }
                if (mutation == 3) {
                    broken.push_back('X');
                }
                if (mutation == 4) {
                    for (std::size_t i = broken.size() - 8; i < broken.size(); ++i) {
                        broken[i] = char(255);
                    }
                }
                save(temp / "bad.mesh", broken);
                rejects(
                    [&] {
                        MeshReader::read(temp / "bad.mesh");
                    },
                    "corrupt native mesh");
            }
        }
        rejects(
            [&] {
                MeshReader::read(temp / "missing.mesh");
            },
            "missing native mesh");
        // Exercise the common custom/warped-hex path even in a build without Gmsh.
        const auto grid =
            MeshGenerator::generate({cfd::MeshType::Cartesian, {0, 1, 0, 1, 0, 1}, {2, 2, 2}});
        Mesh custom(MeshType::Custom, {}, {});
        custom.setName("warped_custom");
        custom.nodes = grid.nodes;
        custom.cells = grid.cells;
        custom.nodes.setDimensions(0, 0, 0);
        custom.cells.setDimensions(0, 0, 0);
        const auto corner = custom.cells.getNodes(0)[6];
        auto coordinate = custom.nodes.getCoordinates(corner);
        coordinate[2] += 0.2;
        custom.nodes.setCoordinates(corner, coordinate);
        MeshConnectivityBuilder::build(custom);
        MeshGeometryProcessor::compute(custom);
        custom.setPatchNames({{42, "warped walls"}});
        for (std::size_t f = 0; f < custom.faces.getNumber(); ++f) {
            custom.faces.setBoundaryPatchId(f, custom.faces.isBoundary(f) ? 42
                                                                          : constants::INVALID_ID);
        }
        MeshValidator::validate(custom);
        bool warped = false;
        for (std::size_t f = 0; f < custom.faces.getNumber(); ++f) {
            warped |=
                custom.faces.getArea(f) > math::norm(custom.faces.getAreaVector(f)) * (1 + 1e-8);
        }
        test::require(warped, "Warped scalar/vector area distinction");
        MeshWriter::write(custom, temp / "custom.mesh");
        equal(custom, MeshReader::read(temp / "custom.mesh"));
        VTKWriter::write(custom, (temp / "custom.vtu").string());
        test::require(VTKReader::read((temp / "custom.vtu").string()).cells.getNumber() ==
                          custom.cells.getNumber(),
                      "Custom VTU visualization");
        std::cout << "PASS full native serialization, all attributes, malformed files and VTU "
                     "round trip\n";
    }

    // ===================================================================================== //
    //                             Complete case workflow tests                              //
    // ===================================================================================== //
    void pipelineTests(const fs::path& temp) {
        for (const auto& name : {"cartesian", "cylindrical", "spherical"}) {
            const auto directory = temp / name;
            fs::create_directories(directory);
            save(directory / "grid.info",
                 contents(fs::path(CFD_SOURCE_DIR) / "examples" / name / "grid.info"));
            const auto mesh = Preprocessor::run(directory);
            equal(mesh, MeshReader::read(directory / (mesh.getName() + ".mesh")));
            const auto native = directory / (mesh.getName() + ".mesh"),
                       vtu = directory / (mesh.getName() + ".vtu");
            const auto oldNative = contents(native), oldVTU = contents(vtu);
            equal(mesh, Preprocessor::run(directory)); // Replace complete existing outputs.
            save(directory / "grid.info", "name=" + mesh.getName() + "\ntype=cartesian\nLx=-1\n");
            rejects(
                [&] {
                    Preprocessor::run(directory);
                },
                "invalid pipeline input");
            test::require(contents(native) == oldNative && contents(vtu) == oldVTU,
                          "Invalid input overwrote existing outputs");
            for (const auto& entry : fs::directory_iterator(directory)) {
                test::require(!entry.path().filename().string().starts_with(".preprocessor-"),
                              "Staging cleanup");
            }
        }
        std::cout << "PASS complete case workflows and existing-output protection\n";
    }

    // ===================================================================================== //
    //                               Optional Gmsh import tests                              //
    // ===================================================================================== //
    void gmshTests(const fs::path& temp) {
        const auto source = fs::path(CFD_SOURCE_DIR);
        rejects(
            [&] {
                GmshImporter::read(temp / "missing.msh");
            },
            "missing Gmsh input");
        if (GmshImporter::available()) {
            const auto mesh = GmshImporter::read(source / "examples/custom/hex.msh");
            MeshValidator::validate(mesh);
            test::require(mesh.cells.getNumber() == 1 && mesh.faces.getNumber() == 6 &&
                              test::approx(volume(mesh), 1),
                          "Gmsh fixture");
            test::require(mesh.getPatchNames().at(7) == "walls", "Gmsh physical name/tag");
            for (std::size_t f = 0; f < 6; ++f) {
                test::require(mesh.faces.getBoundaryPatchId(f) == 7, "Gmsh physical patch");
            }
            MeshWriter::write(mesh, temp / "gmsh.mesh");
            equal(mesh, MeshReader::read(temp / "gmsh.mesh"));
            rejects(
                [&] {
                    GmshImporter::read(source / "tests/mesh/unsupported.msh");
                },
                "tetrahedron");
            save(temp / "hex.msh", contents(source / "examples/custom/hex.msh"));
            save(temp / "grid.info", contents(source / "examples/custom/grid.info"));
            equal(mesh, Preprocessor::run(temp));
            std::cout << "PASS Gmsh API hex import, physical groups, unsupported elements and "
                         "custom pipeline\n";
        } else {
            bool unavailable = false;
            try {
                GmshImporter::read(source / "examples/custom/hex.msh");
            } catch (const std::runtime_error& e) {
                unavailable =
                    std::string(e.what()).find("Gmsh support is unavailable") != std::string::npos;
            }
            test::require(unavailable, "Clear no-SDK diagnostic");
            std::cout
                << "PASS no-Gmsh fallback (Gmsh API integration tests not run: SDK unavailable)\n";
        }
    }
} // namespace

// ===================================================================================== //
//                                  Test group dispatch                                  //
// ===================================================================================== //
int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::runtime_error("Expected test group");
        }
        const std::string group = argv[1];
        const auto temp = fs::current_path() / ("preprocessor-test-" + group);
        fs::create_directories(temp);
        if (group == "config") {
            configTests(temp);
        } else if (group == "generated") {
            generatedTests();
        } else if (group == "native") {
            nativeTests(temp);
        } else if (group == "pipeline") {
            pipelineTests(temp);
        } else if (group == "gmsh") {
            gmshTests(temp);
        } else {
            throw std::runtime_error("Unknown test group");
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
