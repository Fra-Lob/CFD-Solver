#include <cfd/meshio/VTKWriter.hpp>

namespace cfd {

void VTKWriter::write(const Mesh& mesh, const std::string& filename)
{
    const auto numberOfPoints = mesh.nodes.getNumber();
    const auto numberOfCells = mesh.cells.getNumber();

    // Validate before opening the file, so invalid input does not truncate it.
    for (std::size_t point = 0; point < numberOfPoints; ++point) {
        for (const auto coordinate : mesh.nodes.getCoordinates(point)) {
            if (!std::isfinite(coordinate)) {
                throw std::invalid_argument("VTKWriter: node coordinates must be finite");
            }
        }
    }
    for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
        for (const auto node : mesh.cells.getNodes(cell)) {
            if (node >= numberOfPoints) {
                throw std::invalid_argument("VTKWriter: cell references a nonexistent node");
            }
        }
    }

    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("VTKWriter: cannot open file: " + filename);
    }
    // XML numbers always use a decimal point, regardless of the user's locale.
    file.imbue(std::locale::classic());
    file << std::setprecision(std::numeric_limits<double>::max_digits10);

    file << "<?xml version=\"1.0\"?>\n"
         << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n"
         << "  <UnstructuredGrid>\n"
         << "    <Piece NumberOfPoints=\"" << numberOfPoints
         << "\" NumberOfCells=\"" << numberOfCells << "\">\n"
         << "      <Points>\n"
         << "        <DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">\n";

    for (std::size_t point = 0; point < numberOfPoints; ++point) {
        const auto coordinates = mesh.nodes.getCoordinates(point);
        file << "          " << coordinates[0] << ' ' << coordinates[1]
             << ' ' << coordinates[2] << '\n';
    }

    file << "        </DataArray>\n"
         << "      </Points>\n"
         << "      <Cells>\n"
         << "        <DataArray type=\"UInt64\" Name=\"connectivity\" format=\"ascii\">\n";

    for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
        file << "          ";
        for (const auto node : mesh.cells.getNodes(cell)) {
            file << node << ' ';
        }
        file << '\n';
    }

    // Offsets are cumulative end positions in the connectivity array.
    file << "        </DataArray>\n"
         << "        <DataArray type=\"UInt64\" Name=\"offsets\" format=\"ascii\">\n";
    for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
        file << "          " << (cell + 1) * 8 << '\n';
    }

    file << "        </DataArray>\n"
         << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n";
    for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
        file << "          12\n"; // VTK_HEXAHEDRON; same node order as MeshGenerator.
    }

    file << "        </DataArray>\n"
         << "      </Cells>\n"
         << "    </Piece>\n"
         << "  </UnstructuredGrid>\n"
         << "</VTKFile>\n";

    file.close();
    if (!file) {
        throw std::runtime_error("VTKWriter: failed writing file: " + filename);
    }
}

}
