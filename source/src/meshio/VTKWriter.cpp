#include <cfd/meshio/VTKWriter.hpp>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <stdexcept>

#include "VTKBezier.hpp"

namespace cfd {

    // ===================================================================================== //
    //                             ASCII UnstructuredGrid export                             //
    // ===================================================================================== //
    void VTKWriter::write(const Mesh& mesh, const std::string& filename) {
        std::optional<detail::BezierGrid> bezier;
        if (mesh.isCurved()) {
            bezier.emplace(mesh);
        }
        const auto numberOfPoints = bezier ? bezier->points.size() : mesh.nodes.getNumber();
        const auto numberOfCells = mesh.cells.getNumber();

        // Validate before opening the file, so invalid input does not truncate it.
        for (std::size_t point = 0; point < numberOfPoints; ++point) {
            for (const auto coordinate :
                 (bezier ? bezier->points[point] : mesh.nodes.getCoordinates(point))) {
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
             << "  <UnstructuredGrid>\n";
        if (mesh.hasProperties() && mesh.getType() != MeshType::Custom) {
            file << "    <FieldData>\n"
                 << "      <DataArray type=\"UInt8\" Name=\"cfd_mesh_type\" NumberOfTuples=\"1\" "
                    "format=\"ascii\">"
                 << static_cast<unsigned>(mesh.getType()) << "</DataArray>\n"
                 << "      <DataArray type=\"Float64\" Name=\"cfd_mesh_domain\" "
                    "NumberOfComponents=\""
                 << mesh.getDomain().size() << "\" NumberOfTuples=\"1\" format=\"ascii\">";
            for (double value : mesh.getDomain()) {
                file << value << ' ';
            }
            file << "</DataArray>\n"
                 << "      <DataArray type=\"UInt64\" Name=\"cfd_mesh_resolution\" "
                    "NumberOfComponents=\"3\" NumberOfTuples=\"1\" format=\"ascii\">";
            for (auto count : mesh.getResolution()) {
                file << count << ' ';
            }
            file << "</DataArray>\n"
                 << "      <DataArray type=\"UInt8\" Name=\"cfd_mesh_name\" NumberOfTuples=\""
                 << mesh.getName().size() << "\" format=\"ascii\">";
            for (unsigned char character : mesh.getName()) {
                file << unsigned(character) << ' ';
            }
            file << "</DataArray>\n    </FieldData>\n";
        }
        file << "    <Piece NumberOfPoints=\"" << numberOfPoints << "\" NumberOfCells=\""
             << numberOfCells << "\">\n";
        if (bezier) {
            file << "      <PointData RationalWeights=\"RationalWeights\">\n"
                 << "        <DataArray type=\"Float64\" Name=\"RationalWeights\" "
                    "format=\"ascii\">\n";
            for (double weight : bezier->weights) {
                file << "          " << weight << '\n';
            }
            file << "        </DataArray>\n      </PointData>\n";
        }
        file << "      <Points>\n"
             << "        <DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">\n";

        for (std::size_t point = 0; point < numberOfPoints; ++point) {
            const auto coordinates =
                (bezier ? bezier->points[point] : mesh.nodes.getCoordinates(point));
            file << "          " << coordinates[0] << ' ' << coordinates[1] << ' ' << coordinates[2]
                 << '\n';
        }

        file << "        </DataArray>\n"
             << "      </Points>\n"
             << "      <Cells>\n"
             << "        <DataArray type=\"UInt64\" Name=\"connectivity\" format=\"ascii\">\n";

        for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
            file << "          ";
            if (bezier) {
                for (auto node : bezier->cells[cell]) {
                    file << node << ' ';
                }
            } else {
                for (auto node : mesh.cells.getNodes(cell)) {
                    file << node << ' ';
                }
            }
            file << '\n';
        }

        // Offsets are cumulative end positions in the connectivity array.
        file << "        </DataArray>\n"
             << "        <DataArray type=\"UInt64\" Name=\"offsets\" format=\"ascii\">\n";
        for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
            file << "          " << (cell + 1) * (bezier ? 27 : 8) << '\n';
        }

        file << "        </DataArray>\n"
             << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n";
        for (std::size_t cell = 0; cell < numberOfCells; ++cell) {
            file << "          " << (bezier ? 79 : 12) << '\n';
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

} // namespace cfd
