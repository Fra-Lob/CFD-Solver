#include "TestUtils.hpp"

#include <io/CaseReader.hpp>
#include <io/VTKConverter.hpp>
#include <mesh/MeshBuilder.hpp>
#include <meshio/MeshConverter.hpp>
#include <meshio/MeshGenerator.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>

namespace test {

void testCaseReader()
{
    const std::string path = "case-reader-test.info";
    {
        std::ofstream file(path);
        file << "# example case\n"
             << "name = TestCase\n"
             << "type = scalarTransport\n"
             << "mesh = TestMesh\n"
             << "scalar fields = C1, C2\n"
             << "diffusivity = 1.0, 0.5\n"
             << "initialValues = 300.0, 150.0\n"
             << "mode = steady\n"
             << "method = conjugateGradient\n"
             << "tolerance = 1e-10\n"
             << "maxIterations = 10000\n"
             << "xmin\n"
             << "type = Dirichlet, Neumann\n"
             << "value = 100.0, 0.0\n"
             << "xmax\n"
             << "type = Dirichlet, Dirichlet\n"
             << "value = 0.0, 10.0\n"
             << "directory = results\n"
             << "vtk = true\n";
    }

    try {
        auto config = CaseReader::read(path, 12);
        require(config.name == "TestCase", "Case reader simulation name");
        require(config.type == "scalarTransport", "Case reader simulation type");
        require(config.meshFile == "TestMesh", "Case reader mesh file");
        require(config.scalarFields.size() == 2, "Case reader scalar-field collection");
        require(config.scalarFields[0].getName() == "C1", "Case reader first field name");
        require(config.scalarFields[1].getName() == "C2", "Case reader second field name");
        require(approx(config.scalarFields[0].getDiffusivity(), 1.0),
                "Case reader first diffusivity");
        require(approx(config.scalarFields[1].getInitialValue(), 150.0),
                "Case reader second initial value");
        require(config.scalarFields[0].getNumber() == 12, "Scalar field cell count");
        require(approx(config.scalarFields[1].getValue(11), 150.0),
                "Scalar field initial-value initialization");
        require(config.linearSolver == "conjugateGradient", "Case reader linear solver");
        require(approx(config.tolerance, 1e-10), "Case reader solver tolerance");
        require(config.maxIterations == 10000, "Case reader maximum iterations");
        require(config.writeVtk, "Case reader VTK output flag");
        require(config.outputDirectory == "results", "Case reader output directory");
        require(config.scalarFields[0].getBoundaryConditions().get(
                    static_cast<std::size_t>(CartesianPatch::XMin)).type
                    == BoundaryType::Dirichlet,
                "Case reader field boundary type");
        require(approx(config.scalarFields[1].getBoundaryConditions().get(
                           static_cast<std::size_t>(CartesianPatch::XMax)).value, 10.0),
                "Case reader field boundary value");

        auto mesh = MeshGenerator::generate({1.0, 1.0, 1.0, 3, 2, 2});
        MeshBuilder::computeConnectivity(mesh);
        MeshBuilder::computeFullGeometry(mesh);
        MeshBuilder::computeBoundaryPatch(mesh);
        const std::string meshPath = "TestMesh.mesh";
        MeshConverter::write(mesh, meshPath);
        const auto loaded = CaseReader::load(path);
        require(loaded.mesh.cells.getNumber() == 12,
                "Case reader loads the configured mesh");
        require(loaded.config.scalarFields.size() == 2,
                "Case reader initializes fields after mesh loading");
        require(loaded.config.scalarFields[0].getNumber() == 12,
                "Loaded scalar fields match mesh cells");
        std::filesystem::remove(meshPath);

        config.scalarFields[0].fill(25.0);
        config.scalarFields[1].fill(75.0);
        const std::string vtkPath = "case-reader-fields.vtu";
        VTKConverter::write(mesh, vtkPath, config);
        std::ifstream vtk(vtkPath);
        const std::string vtkText{
            std::istreambuf_iterator<char>(vtk),
            std::istreambuf_iterator<char>()
        };
        require(vtkText.find("Name=\"C1\"") != std::string::npos,
                "VTK converter writes C1 scalar field");
        require(vtkText.find("Name=\"C2\"") != std::string::npos,
                "VTK converter writes C2 scalar field");
        std::filesystem::remove(vtkPath);
    } catch (...) {
        std::filesystem::remove(path);
        std::filesystem::remove("TestMesh.mesh");
        std::filesystem::remove("case-reader-fields.vtu");
        throw;
    }
    std::filesystem::remove(path);
}

}
