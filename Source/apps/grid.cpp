
#include <meshio/MeshGenerator.hpp>
#include <meshio/MeshConverter.hpp>
#include <io/VTKConverter.hpp>
#include <mesh/MeshBuilder.hpp>

#include <iostream>

int main() {

    std::string meshName = "Box";

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

    MeshConverter::write(mesh, meshName + ".mesh");
    VTKConverter::write(mesh, meshName  + ".vtu");

    std::cout << "Mesh exported successfully to ";
    std::cout << meshName << ".mesh and " << meshName << ".vtu\n";

    return 0;
}