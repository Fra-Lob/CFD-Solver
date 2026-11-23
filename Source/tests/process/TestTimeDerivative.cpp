#include "TestUtils.hpp"

#include <io/OutputConfig.hpp>
#include <io/SimulationConfig.hpp>
#include <mesh/MeshBuilder.hpp>
#include <meshio/MeshGenerator.hpp>
#include <numerics/LinearSystem.hpp>
#include <process/Runner.hpp>
#include <process/TimeDerivative.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace test {

void testTimeDerivative()
{
    auto mesh = MeshGenerator::generate({1.0, 1.0, 1.0, 1, 1, 1});
    MeshBuilder::computeConnectivity(mesh);
    MeshBuilder::computeFullGeometry(mesh);
    MeshBuilder::computeBoundaryPatch(mesh);

    ScalarField oldField("C", 1, 4.0);
    LinearSystem system(1);
    TimeDerivative::assemble(mesh, 2.0, 0.5, oldField, system);
    system.finalize();

    require(approx(system.getMatrix().coeff(0, 0), 4.0),
            "Time derivative adds rho*volume/dt to the diagonal");
    require(approx(system.getRhs()[0], 16.0),
            "Time derivative adds the previous field to the RHS");
}

void testTransientRunnerAndOutputSchedule()
{
    auto mesh = MeshGenerator::generate({1.0, 1.0, 1.0, 1, 1, 1});
    MeshBuilder::computeConnectivity(mesh);
    MeshBuilder::computeFullGeometry(mesh);
    MeshBuilder::computeBoundaryPatch(mesh);

    BoundaryConditions boundaries;
    for (const auto patch : {
             CartesianPatch::XMin, CartesianPatch::XMax,
             CartesianPatch::YMin, CartesianPatch::YMax,
             CartesianPatch::ZMin, CartesianPatch::ZMax}) {
        boundaries.set(static_cast<std::size_t>(patch), BoundaryType::Neumann, 0.0);
    }

    SimulationConfig config;
    config.timeStep = 0.25;
    config.timeSteps = 3;
    config.density = 1.0;
    config.tolerance = 1e-12;
    config.maxIterations = 100;
    ScalarField field("constant", 1, 7.0);
    field.setDiffusivity(1.0);
    field.setBoundaryConditions(boundaries);
    config.scalarFields.push_back(field);

    OutputConfig output;
    output.fileName = "transient-runner-test.vtu";
    output.writeVtk = true;
    output.vtkInterval = 2;
    output.writeCsv = true;
    output.csvInterval = 2;
    output.csvLines.push_back({{0.0, 0.5, 0.5}, {1.0, 0.5, 0.5}, 2});

    const std::string vtkAtTwo = "transient-runner-testt2.vtu";
    const std::string vtkAtThree = "transient-runner-testt3.vtu";
    const std::string vtkAtOne = "transient-runner-testt1.vtu";
    const std::string csvAtTwo = "line1t2.csv";
    const std::string csvAtThree = "line1t3.csv";
    std::filesystem::remove(vtkAtOne);
    try {
        SimulationRunner::run(mesh, config, output);
        require(std::filesystem::exists(vtkAtTwo),
                "Transient output is written at configured intervals");
        require(std::filesystem::exists(vtkAtThree),
                "Transient output includes the final timestep");
        require(!std::filesystem::exists(vtkAtOne),
                "Transient output skips timesteps outside the interval");
        require(std::filesystem::exists(csvAtTwo) && std::filesystem::exists(csvAtThree),
                "CSV output follows the same interval and final-step schedule");
        require(approx(config.scalarFields[0].getValue(0), 7.0),
                "Implicit transient solve preserves a constant Neumann field");

        std::ifstream csv(csvAtThree);
        const std::string text{
            std::istreambuf_iterator<char>(csv),
            std::istreambuf_iterator<char>()
        };
        require(text.find("point,constant") == 0,
                "Timestep CSV contains scalar-field headers");
    } catch (...) {
        std::filesystem::remove(vtkAtTwo);
        std::filesystem::remove(vtkAtThree);
        std::filesystem::remove(vtkAtOne);
        std::filesystem::remove(csvAtTwo);
        std::filesystem::remove(csvAtThree);
        throw;
    }

    std::filesystem::remove(vtkAtTwo);
    std::filesystem::remove(vtkAtThree);
    std::filesystem::remove(vtkAtOne);
    std::filesystem::remove(csvAtTwo);
    std::filesystem::remove(csvAtThree);
}

}
