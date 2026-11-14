#include "TestUtils.hpp"

#include <cfd/meshio/VTKReader.hpp>
#include <cfd/meshio/VTKWriter.hpp>
#include <cfd/meshio/MeshGenerator.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

namespace {

void compare(const cfd::Mesh& a, const cfd::Mesh& b)
{
    test::require(a.getType() == b.getType() && a.getDimension() == b.getDimension() &&
                  a.getDomain() == b.getDomain(), "Mesh properties round trip");
    test::require(a.nodes.getNumber() == b.nodes.getNumber(), "Node count");
    test::require(a.cells.getNumber() == b.cells.getNumber(), "Cell count");
    test::require(a.faces.getNumber() == b.faces.getNumber(), "Face count");
    for (std::size_t p = 0; p < a.nodes.getNumber(); ++p)
        test::require(a.nodes.getCoordinates(p) == b.nodes.getCoordinates(p), "Coordinates round trip");
    for (std::size_t c = 0; c < a.cells.getNumber(); ++c) {
        test::require(a.cells.getNodes(c) == b.cells.getNodes(c), "Cell nodes");
        test::require(a.cells.getFaces(c) == b.cells.getFaces(c), "Cell faces");
        test::require(test::approx(a.cells.getVolume(c), b.cells.getVolume(c)), "Cell volume");
        for (unsigned d = 0; d < 3; ++d)
            test::require(test::approx(a.cells.getCenter(c)[d], b.cells.getCenter(c)[d]), "Cell center");
    }
    for (std::size_t f = 0; f < a.faces.getNumber(); ++f) {
        test::require(a.faces.getNodes(f) == b.faces.getNodes(f), "Face nodes");
        test::require(a.faces.getOwner(f) == b.faces.getOwner(f), "Face owner");
        test::require(a.faces.getNeighbor(f) == b.faces.getNeighbor(f), "Face neighbor");
        test::require(a.faces.isBoundary(f) == b.faces.isBoundary(f), "Boundary detection");
        test::require(a.faces.getBoundaryPatchId(f) == b.faces.getBoundaryPatchId(f), "Boundary patch ID");
        test::require(test::approx(a.faces.getArea(f), b.faces.getArea(f)), "Face area");
        for (unsigned d = 0; d < 3; ++d) {
            test::require(test::approx(a.faces.getCenter(f)[d], b.faces.getCenter(f)[d]), "Face center");
            test::require(test::approx(a.faces.getAreaVector(f)[d], b.faces.getAreaVector(f)[d]), "Face area vector");
        }
    }
}

void replace(std::string& text, const std::string& before, const std::string& after)
{
    const auto pos = text.find(before);
    test::require(pos != std::string::npos, "Test fixture replacement");
    text.replace(pos, before.size(), after);
}

void save(const std::string& path, const std::string& text)
{
    std::ofstream file(path);
    file << text;
    file.close();
    test::require(bool(file), "Write fixture");
}

void rejects(const std::string& path)
{
    try { (void)cfd::VTKReader::read(path); }
    catch (const std::runtime_error&) { return; }
    throw std::runtime_error("Expected rejection of invalid VTU");
}

} // namespace

int main()
{
    const std::string path = "vtk-io-test.vtu";
    try {
        for (const cfd::MeshConfig config : {cfd::MeshConfig{cfd::MeshType::Cartesian, 3, {1., 1., 1.}, 1, 1, 1},
                                           cfd::MeshConfig{cfd::MeshType::Cartesian, 3, {2., 3., 4.}, 4, 3, 2}}) {
            auto original = cfd::MeshGenerator::generate(config);
            cfd::VTKWriter::write(original, path);
            const auto loaded = cfd::VTKReader::read(path);
            cfd::MeshConnectivityBuilder::build(original);
            cfd::MeshGeometryProcessor::compute(original);
            cfd::MeshBoundaryPatchBuilder::buildCartesian(original);
            compare(original, loaded);
            test::require(loaded.cells.getNx() == config.Nx && loaded.cells.getNy() == config.Ny &&
                          loaded.cells.getNz() == config.Nz, "Structured cell dimensions");
            test::require(loaded.nodes.getNx() == config.Nx + 1 && loaded.nodes.getNy() == config.Ny + 1 &&
                          loaded.nodes.getNz() == config.Nz + 1, "Structured node dimensions");
        }

        auto mesh = cfd::MeshGenerator::generate({cfd::MeshType::Cartesian, 3, {2., 3., 4.}, 2, 1, 1});
        const auto first = mesh.cells.getNodes(0);
        mesh.cells.setNodes(0, mesh.cells.getNodes(1));
        mesh.cells.setNodes(1, first);
        cfd::VTKWriter::write(mesh, path);
        auto loaded = cfd::VTKReader::read(path);
        test::require(loaded.cells.getNx() == 0, "Do not invent dimensions for reordered cells");
        cfd::MeshConnectivityBuilder::build(mesh);
        cfd::MeshGeometryProcessor::compute(mesh);
        cfd::MeshBoundaryPatchBuilder::buildCartesian(mesh);
        compare(mesh, loaded);

        mesh = cfd::MeshGenerator::generate({cfd::MeshType::Cartesian, 3, {1., 1., 1.}, 1, 1, 1});
        cfd::VTKWriter::write(mesh, path);
        std::ifstream input(path);
        const std::string valid{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        input.close();
        auto variant = valid;
        replace(variant, "NumberOfPoints=\"8\" NumberOfCells=\"1\"", "NumberOfCells='1'\n NumberOfPoints = '8'");
        replace(variant, "<Points>", "<!-- mesh -->\n<Points>");
        replace(variant, "      <Cells>", "      <PointData/>\n      <CellData/>\n      <Cells>");
        save(path, variant);
        test::require(cfd::VTKReader::read(path).faces.getNumber() == 6, "XML whitespace, quotes, comments");

        // Older/external files remain readable, but domain properties must be supplied
        // explicitly before the Cartesian boundaries can be classified.
        auto legacy = valid;
        const auto beginField = legacy.find("    <FieldData>");
        const auto endField = legacy.find("    </FieldData>\n") + std::string("    </FieldData>\n").size();
        legacy.erase(beginField, endField - beginField);
        save(path, legacy);
        loaded = cfd::VTKReader::read(path);
        test::require(!loaded.hasProperties(), "Do not invent legacy domain properties");
        for (std::size_t f = 0; f < loaded.faces.getNumber(); ++f)
            test::require(loaded.faces.getBoundaryPatchId(f) == cfd::constants::INVALID_ID, "Legacy patch must be unknown");
        loaded = cfd::VTKReader::read(path, cfd::MeshType::Cartesian, 3, {1., 1., 1.});
        cfd::MeshConnectivityBuilder::build(mesh);
        cfd::MeshGeometryProcessor::compute(mesh);
        cfd::MeshBoundaryPatchBuilder::buildCartesian(mesh);
        compare(mesh, loaded);
        save(path, valid);
        compare(mesh, cfd::VTKReader::read(path, cfd::MeshType::Cartesian, 3, {1., 1., 1.}));
        bool conflict = false;
        try { cfd::VTKReader::read(path, cfd::MeshType::Cartesian, 3, {2., 1., 1.}); }
        catch (const std::runtime_error&) { conflict = true; }
        test::require(conflict, "Conflicting supplied properties must be rejected");

        // Retain support for legacy sheared hexahedra without declaring an
        // incompatible zero-origin rectangular domain.
        cfd::Mesh sheared;
        sheared.nodes = mesh.nodes;
        sheared.cells = mesh.cells;
        for (std::size_t c = 0; c < sheared.cells.getNumber(); ++c)
            sheared.cells.setFaces(c, {cfd::constants::INVALID_ID, cfd::constants::INVALID_ID,
                                     cfd::constants::INVALID_ID, cfd::constants::INVALID_ID,
                                     cfd::constants::INVALID_ID, cfd::constants::INVALID_ID});
        for (std::size_t p = 0; p < sheared.nodes.getNumber(); ++p) {
            auto xyz = sheared.nodes.getCoordinates(p);
            xyz[0] += 0.15 * xyz[1];
            sheared.nodes.setCoordinates(p, xyz);
        }
        cfd::VTKWriter::write(sheared, path);
        loaded = cfd::VTKReader::read(path);
        cfd::MeshConnectivityBuilder::build(sheared);
        cfd::MeshGeometryProcessor::compute(sheared);
        compare(sheared, loaded);
        test::require(loaded.nodes.getNx() == 2 && loaded.nodes.getNy() == 2 && loaded.nodes.getNz() == 2,
                      "Recover legacy sheared dimensions from topology");

        const std::pair<std::string, std::string> invalid[] = {
            {"format=\"ascii\"", "format=\"binary\""},
            {"          12\n", "          10\n"},
            {"          8\n", "          7\n"},
            {"0 1 3 2 4 5 7 6", "99 1 3 2 4 5 7 6"},
            {"0 1 3 2 4 5 7 6", "-1 1 3 2 4 5 7 6"},
            {"0 1 3 2 4 5 7 6", "0 0 3 2 4 5 7 6"},
            {"0 1 3 2 4 5 7 6", "0 1 3 2 4 5 7"},
            {"0 1 3 2 4 5 7 6", "0 1 3 2 4 5 7 6 0"},
            {"NumberOfPoints=\"8\"", "NumberOfPoints=\"18446744073709551616\""},
            {"</Cells>", "</Points>"},
            {"</VTKFile>", ""},
            {"</Piece>", "</Piece><Piece NumberOfPoints='0' NumberOfCells='0'/>"},
            {"<Cells>", "<Cells><DataArray Name='offsets' type='UInt64' format='ascii'>8</DataArray>"},
            {"          0 0 0", "          nan 0 0"},
            {"Name=\"cfd_mesh_type\"", "Name=\"missing_type\""},
            {"Name=\"cfd_mesh_type\" NumberOfTuples=\"1\" format=\"ascii\">1",
             "Name=\"cfd_mesh_type\" NumberOfTuples=\"1\" format=\"ascii\">99"},
            {"Name=\"cfd_mesh_type\" NumberOfTuples=\"1\" format=\"ascii\">1",
             "Name=\"cfd_mesh_type\" NumberOfTuples=\"1\" format=\"ascii\">2"},
            {"Name=\"cfd_mesh_domain\" NumberOfComponents=\"3\" NumberOfTuples=\"1\" format=\"ascii\">1 1 1",
             "Name=\"cfd_mesh_domain\" NumberOfComponents=\"3\" NumberOfTuples=\"1\" format=\"ascii\">2 1 1"},
            {"Name=\"cfd_mesh_domain\" NumberOfComponents=\"3\" NumberOfTuples=\"1\" format=\"ascii\">1 1 1",
             "Name=\"cfd_mesh_domain\" NumberOfComponents=\"3\" NumberOfTuples=\"1\" format=\"ascii\">0 1 1"}
        };
        for (const auto& [before, after] : invalid) {
            variant = valid;
            replace(variant, before, after);
            save(path, variant);
            rejects(path);
        }
        cfd::VTKWriter::write({}, path);
        loaded = cfd::VTKReader::read(path);
        test::require(loaded.nodes.getNumber() == 0 && loaded.faces.getNumber() == 0 && loaded.cells.getNumber() == 0,
                      "Empty mesh");
        std::filesystem::remove(path);
        rejects(path); // Missing file
        std::cout << "All VTK IO tests passed!\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove(path);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
