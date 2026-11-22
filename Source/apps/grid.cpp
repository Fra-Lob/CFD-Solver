
#include <cfd/meshio/MeshGenerator.hpp>
#include <cfd/meshio/MeshConverter.hpp>
#include <cfd/meshio/VTKConverter.hpp>
#include <cfd/mesh/MeshBuilder.hpp>

#include <iostream>

int main()
{
    // Mesh configuration
    MeshConfig config{
        2.0, 1.0, 1.0,
        10, 5, 5
    };

    // Generate mesh
    auto mesh = MeshGenerator::generate(config);

    // Build connectivity
    MeshBuilder::computeConnectivity(mesh);
    MeshBuilder::computeFullGeometry(mesh);
    MeshBuilder::computeBoundaryPatch(mesh);

    MeshConverter::write(mesh, "mesh.mesh");
    VTKConverter::write(mesh, "mesh.vtu");

    std::cout << "Mesh exported successfully to mesh.mesh and mesh.vtu\n";

    return 0;
}