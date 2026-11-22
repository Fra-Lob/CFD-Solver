
#include <cfd/meshio/MeshGenerator.hpp>
#include <cfd/meshio/MeshConverter.hpp>
#include <cfd/meshio/VTKConverter.hpp>
#include <cfd/mesh/MeshBuilder.hpp>

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
    cfd::MeshBuilder::computeConnectivity(mesh);
    cfd::MeshBuilder::computeFullGeometry(mesh);
    cfd::MeshBuilder::computeBoundaryPatch(mesh);

    cfd::MeshConverter::write(mesh, "mesh.mesh");
    cfd::VTKConverter::write(mesh, "mesh.vtu");

    std::cout << "Mesh exported successfully to mesh.mesh and mesh.vtu\n";

    return 0;
}