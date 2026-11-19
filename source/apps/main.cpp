
#include <cfd/meshio/MeshReader.hpp>

#include <iostream>

// ===================================================================================== //
//                            Native mesh loading application                            //
// ===================================================================================== //
int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: Main <completed.mesh>\n";
        return 1;
    }
    try {
        std::cout << "Reading completed native mesh: " << argv[1] << std::endl;
        const auto mesh = cfd::MeshReader::read(argv[1]);
        std::cout << "Loaded " << mesh.getName() << ": " << mesh.cells.getNumber()
                  << " cells; stored topology and geometry ready\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
