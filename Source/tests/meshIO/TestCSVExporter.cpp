#include "TestUtils.hpp"

#include <io/CSVExporter.hpp>
#include <mesh/MeshBuilder.hpp>
#include <meshio/MeshGenerator.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace test {

void testCSVExporter()
{
    auto mesh = MeshGenerator::generate({2.0, 1.0, 1.0, 2, 2, 2});
    MeshBuilder::computeConnectivity(mesh);
    MeshBuilder::computeFullGeometry(mesh);
    MeshBuilder::computeBoundaryPatch(mesh);

    SimulationConfig config;
    OutputConfig output;
    output.writeCsv = true;
    output.csvLines.push_back({
        {0.0, 0.5, 0.5},
        {2.0, 0.5, 0.5},
        3
    });
    output.csvLines.push_back({
        {0.5, 0.0, 0.5},
        {0.5, 1.0, 0.5},
        3
    });

    ScalarField field("linear field", mesh.cells.getNumber(), 0.0);
    for (std::size_t k = 0; k < mesh.cells.getNz(); ++k) {
        for (std::size_t j = 0; j < mesh.cells.getNy(); ++j) {
            for (std::size_t i = 0; i < mesh.cells.getNx(); ++i) {
                const auto cell = mesh.cells.getIdFromIndices(i, j, k);
                field.setValue(cell, mesh.cells.getCenter(cell)[0]);
            }
        }
    }

    BoundaryConditions boundaries;
    boundaries.set(static_cast<std::size_t>(CartesianPatch::XMin),
                   BoundaryType::Dirichlet, 0.0);
    boundaries.set(static_cast<std::size_t>(CartesianPatch::XMax),
                   BoundaryType::Dirichlet, 2.0);
    for (const auto patch : {
             CartesianPatch::YMin, CartesianPatch::YMax,
             CartesianPatch::ZMin, CartesianPatch::ZMax}) {
        boundaries.set(static_cast<std::size_t>(patch), BoundaryType::Neumann, 0.0);
    }
    field.setBoundaryConditions(boundaries);
    config.scalarFields.push_back(field);

    const std::string filename = "line1.csv";
    try {
        CSVExporter::write(mesh, config, output);
        std::ifstream file(filename);
        require(static_cast<bool>(file), "CSV exporter creates line1.csv");
        std::string header;
        std::getline(file, header);
        require(header == "point,linear field", "CSV header contains point and field name");

        std::string row;
        std::array<double, 3> values{};
        std::size_t count = 0;
        while (std::getline(file, row)) {
            std::istringstream parser(row);
            std::string point;
            std::string value;
            require(static_cast<bool>(std::getline(parser, point, ',')),
                    "CSV row contains a point index");
            require(static_cast<bool>(std::getline(parser, value, ',')),
                    "CSV row contains a field value");
            require(point == std::to_string(count + 1), "CSV point indices are sequential");
            values.at(count++) = std::stod(value);
        }
        require(count == 3, "CSV exporter writes configured resolution");
        require(approx(values[0], 0.0), "CSV uses Dirichlet face value at x-min");
        require(approx(values[1], 1.0), "CSV linearly interpolates cell-center values");
        require(approx(values[2], 2.0), "CSV uses Dirichlet face value at x-max");

        std::ifstream neumannFile("line2.csv");
        require(static_cast<bool>(neumannFile), "CSV exporter creates one file per line");
        std::getline(neumannFile, header);
        require(header == "point,linear field", "Second CSV file contains the field header");
        count = 0;
        while (std::getline(neumannFile, row)) {
            std::istringstream parser(row);
            std::string point;
            std::string value;
            std::getline(parser, point, ',');
            std::getline(parser, value, ',');
            require(approx(std::stod(value), 0.5),
                    "CSV uses owner-cell values on Neumann faces");
            ++count;
        }
        require(count == 3, "Second CSV file writes configured resolution");
    } catch (...) {
        std::filesystem::remove(filename);
        std::filesystem::remove("line2.csv");
        throw;
    }
    std::filesystem::remove(filename);
    std::filesystem::remove("line2.csv");
}

}
