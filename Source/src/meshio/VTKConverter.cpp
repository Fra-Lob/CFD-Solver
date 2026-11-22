#include <meshio/VTKConverter.hpp>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <set>
#include <stdexcept>

namespace {

std::string escapeXml(const std::string& value)
{
    std::string escaped;
    for (const char character : value) {
        switch (character) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped += character; break;
        }
    }
    return escaped;
}

void writeArrayHeader(
    std::ostream& file,
    const std::string& type,
    const std::string& name,
    std::size_t components = 1
)
{
    file << "        <DataArray type=\"" << type << "\"";
    if (!name.empty()) file << " Name=\"" << escapeXml(name) << "\"";
    if (components != 1) file << " NumberOfComponents=\"" << components << "\"";
    file << " format=\"ascii\">\n";
}

void validateMesh(
    const Mesh& mesh,
    const std::vector<CellScalarData>& cellScalars
)
{
    const auto numberOfPoints = mesh.nodes.getNumber();
    const auto numberOfCells = mesh.cells.getNumber();
    if (numberOfCells > std::numeric_limits<std::size_t>::max() / 8) {
        throw std::invalid_argument("VTKConverter: cell connectivity size overflows");
    }

    for (std::size_t point = 0; point < numberOfPoints; ++point) {
        for (const auto coordinate : mesh.nodes.getCoordinates(point)) {
            if (!std::isfinite(coordinate)) {
                throw std::invalid_argument("VTKConverter: node coordinates must be finite");
            }
        }
    }
    for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
        for (const auto node : mesh.cells.getNodes(cell)) {
            if (node >= numberOfPoints) {
                throw std::invalid_argument("VTKConverter: cell references a nonexistent node");
            }
        }
    }

    std::set<std::string> names{"boundaryPatchIds"};
    for (const auto& scalar : cellScalars) {
        if (scalar.name.empty() || !names.insert(scalar.name).second) {
            throw std::invalid_argument("VTKConverter: cell scalar names must be nonempty and unique");
        }
        if (scalar.values.size() != numberOfCells) {
            throw std::invalid_argument("VTKConverter: cell scalar count must match the number of cells");
        }
        for (const auto value : scalar.values) {
            if (!std::isfinite(value)) {
                throw std::invalid_argument("VTKConverter: cell scalar values must be finite");
            }
        }
    }
    for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
        for (const auto face : mesh.cells.getFaces(cell)) {
            if (face != constants::invalidId && face >= mesh.faces.getNumber()) {
                throw std::invalid_argument("VTKConverter: cell references a nonexistent face");
            }
        }
    }
}

void writePoints(std::ostream& file, const Mesh& mesh)
{
    file << "      <Points>\n";
    writeArrayHeader(file, "Float64", {}, 3);
    for (std::size_t point = 0; point < mesh.nodes.getNumber(); ++point) {
        const auto coordinates = mesh.nodes.getCoordinates(point);
        file << "          " << coordinates[0] << ' ' << coordinates[1]
             << ' ' << coordinates[2] << '\n';
    }
    file << "        </DataArray>\n"
         << "      </Points>\n";
}

void writeCells(std::ostream& file, const Mesh& mesh)
{
    const auto numberOfCells = mesh.cells.getNumber();
    file << "      <Cells>\n";
    writeArrayHeader(file, "UInt64", "connectivity");
    for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
        file << "          ";
        for (const auto node : mesh.cells.getNodes(cell)) file << node << ' ';
        file << '\n';
    }
    file << "        </DataArray>\n";

    writeArrayHeader(file, "UInt64", "offsets");
    for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
        file << "          " << (cell + 1) * 8 << '\n';
    }
    file << "        </DataArray>\n";

    writeArrayHeader(file, "UInt8", "types");
    for (std::size_t cell = 0; cell < numberOfCells; ++cell) file << "          12\n";
    file << "        </DataArray>\n"
         << "      </Cells>\n";
}

void writeCellData(
    std::ostream& file,
    const Mesh& mesh,
    const std::vector<CellScalarData>& cellScalars
)
{
    file << "      <CellData>\n";
    writeArrayHeader(file, "UInt64", "boundaryPatchIds", 6);
    for (std::size_t cell = 0; cell < mesh.cells.getNumber(); ++cell) {
        for (const auto face : mesh.cells.getFaces(cell)) {
            const auto patch = face == constants::invalidId
                ? constants::invalidId
                : mesh.faces.getBoundaryPatchId(face);
            file << patch << ' ';
        }
        file << '\n';
    }
    file << "        </DataArray>\n";

    for (const auto& scalar : cellScalars) {
        writeArrayHeader(file, "Float64", scalar.name);
        for (const auto value : scalar.values) file << "          " << value << '\n';
        file << "        </DataArray>\n";
    }
    file << "      </CellData>\n";
}

} // namespace

void VTKConverter::write(
    const Mesh& mesh,
    const std::string& filename,
    const std::vector<CellScalarData>& cellScalars
)
{
    validateMesh(mesh, cellScalars);

    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("VTKConverter: cannot open file: " + filename);
    }
    file.imbue(std::locale::classic());
    file << std::setprecision(std::numeric_limits<double>::max_digits10);

    file << "<?xml version=\"1.0\"?>\n"
         << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n"
         << "  <UnstructuredGrid>\n"
         << "    <Piece NumberOfPoints=\"" << mesh.nodes.getNumber()
         << "\" NumberOfCells=\"" << mesh.cells.getNumber() << "\">\n";

    writePoints(file, mesh);
    writeCells(file, mesh);
    writeCellData(file, mesh, cellScalars);

    file << "    </Piece>\n"
         << "  </UnstructuredGrid>\n"
         << "</VTKFile>\n";
    file.close();
    if (!file) {
        throw std::runtime_error("VTKConverter: failed writing file: " + filename);
    }
}