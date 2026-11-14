
#include <cfd/meshio/MeshGenerator.hpp>
#include <cfd/meshio/VTKWriter.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>

#include <iostream>

int main()
{
    // Mesh configuration
    cfd::MeshConfig config{
        2.0, 1.0, 1.0,
        10, 5, 5
    };

    // Generate mesh
    auto mesh = cfd::MeshGenerator::generate(config);

    // Build connectivity
    cfd::MeshConnectivityBuilder::build(mesh);

    // Compute geometry
    cfd::MeshGeometryProcessor::computeFaces(mesh);
    cfd::MeshGeometryProcessor::computeCells(mesh);

    // Export mesh
    cfd::VTKWriter::write(mesh, "mesh.vtu");

    std::cout << "Mesh exported successfully to mesh.vtu\n";

    return 0;
}