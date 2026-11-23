#include <io/CSVExporter.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Point = std::array<double, 3>;

struct Bracket {
    std::size_t lower;
    std::size_t upper;
    double weight;
};

Bracket bracket(
    const std::vector<double>& coordinates,
    double value,
    bool extrapolate = false
)
{
    if (coordinates.empty()) {
        throw std::runtime_error("CSVExporter: interpolation axis is empty");
    }
    if (coordinates.size() == 1) return {0, 0, 0.0};
    if (value <= coordinates.front()) {
        if (!extrapolate) return {0, 0, 0.0};
        return {
            0, 1,
            (value - coordinates[0]) / (coordinates[1] - coordinates[0])
        };
    }
    if (value >= coordinates.back()) {
        const auto upper = coordinates.size() - 1;
        if (!extrapolate) return {upper, upper, 0.0};
        const auto lower = upper - 1;
        return {
            lower, upper,
            (value - coordinates[lower]) /
                (coordinates[upper] - coordinates[lower])
        };
    }
    const auto upper = static_cast<std::size_t>(
        std::upper_bound(coordinates.begin(), coordinates.end(), value) -
        coordinates.begin()
    );
    const auto lower = upper - 1;
    return {
        lower, upper,
        (value - coordinates[lower]) /
            (coordinates[upper] - coordinates[lower])
    };
}

struct BoundaryPlane {
    std::size_t uAxis;
    std::size_t vAxis;
    std::vector<double> uCoordinates;
    std::vector<double> vCoordinates;
    std::vector<double> values;

    double valueAt(double u, double v) const
    {
        const auto uBracket = bracket(uCoordinates, u, true);
        const auto vBracket = bracket(vCoordinates, v, true);
        const auto at = [&](std::size_t iu, std::size_t iv) {
            return values[iu * vCoordinates.size() + iv];
        };
        const double lower =
            at(uBracket.lower, vBracket.lower) * (1.0 - vBracket.weight) +
            at(uBracket.lower, vBracket.upper) * vBracket.weight;
        const double upper =
            at(uBracket.upper, vBracket.lower) * (1.0 - vBracket.weight) +
            at(uBracket.upper, vBracket.upper) * vBracket.weight;
        return lower * (1.0 - uBracket.weight) +
               upper * uBracket.weight;
    }
};

struct StructuredGrid {
    std::array<std::size_t, 3> cellCounts;
    std::array<std::vector<double>, 3> cellCenters;
    std::array<std::vector<double>, 3> sampleCoordinates;
    Point lower;
    Point upper;
    double tolerance;
};

double coordinateTolerance(const Point& lower, const Point& upper)
{
    double scale = 1.0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        scale = std::max(scale, std::abs(lower[axis]));
        scale = std::max(scale, std::abs(upper[axis]));
    }
    return 1e-10 * scale;
}

StructuredGrid makeGrid(const Mesh& mesh)
{
    StructuredGrid grid;
    grid.cellCounts = {
        mesh.cells.getNx(), mesh.cells.getNy(), mesh.cells.getNz()
    };
    const std::array<std::size_t, 3> nodeCounts = {
        mesh.nodes.getNx(), mesh.nodes.getNy(), mesh.nodes.getNz()
    };
    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (grid.cellCounts[axis] == 0 ||
            nodeCounts[axis] != grid.cellCounts[axis] + 1) {
            throw std::invalid_argument(
                "CSVExporter: sampling requires structured mesh dimensions"
            );
        }
    }

    const auto nodeAt = [&](std::size_t i, std::size_t j, std::size_t k) {
        return mesh.nodes.getCoordinates(mesh.nodes.getIdFromIndices(i, j, k));
    };
    grid.lower = nodeAt(0, 0, 0);
    grid.upper = nodeAt(nodeCounts[0] - 1, nodeCounts[1] - 1, nodeCounts[2] - 1);
    grid.tolerance = coordinateTolerance(grid.lower, grid.upper);

    for (std::size_t i = 0; i < nodeCounts[0]; ++i) {
        grid.sampleCoordinates[0].push_back(nodeAt(i, 0, 0)[0]);
    }
    for (std::size_t j = 0; j < nodeCounts[1]; ++j) {
        grid.sampleCoordinates[1].push_back(nodeAt(0, j, 0)[1]);
    }
    for (std::size_t k = 0; k < nodeCounts[2]; ++k) {
        grid.sampleCoordinates[2].push_back(nodeAt(0, 0, k)[2]);
    }

    for (std::size_t k = 0; k < nodeCounts[2]; ++k) {
        for (std::size_t j = 0; j < nodeCounts[1]; ++j) {
            for (std::size_t i = 0; i < nodeCounts[0]; ++i) {
                const auto point = nodeAt(i, j, k);
                const std::array<std::size_t, 3> index{i, j, k};
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    if (std::abs(point[axis] -
                                 grid.sampleCoordinates[axis][index[axis]]) >
                        grid.tolerance) {
                        throw std::invalid_argument(
                            "CSVExporter: sampling requires an axis-aligned mesh"
                        );
                    }
                }
            }
        }
    }

    for (std::size_t i = 0; i < grid.cellCounts[0]; ++i) {
        grid.cellCenters[0].push_back(
            mesh.cells.getCenter(mesh.cells.getIdFromIndices(i, 0, 0))[0]
        );
    }
    for (std::size_t j = 0; j < grid.cellCounts[1]; ++j) {
        grid.cellCenters[1].push_back(
            mesh.cells.getCenter(mesh.cells.getIdFromIndices(0, j, 0))[1]
        );
    }
    for (std::size_t k = 0; k < grid.cellCounts[2]; ++k) {
        grid.cellCenters[2].push_back(
            mesh.cells.getCenter(mesh.cells.getIdFromIndices(0, 0, k))[2]
        );
    }

    for (std::size_t k = 0; k < grid.cellCounts[2]; ++k) {
        for (std::size_t j = 0; j < grid.cellCounts[1]; ++j) {
            for (std::size_t i = 0; i < grid.cellCounts[0]; ++i) {
                const auto center = mesh.cells.getCenter(
                    mesh.cells.getIdFromIndices(i, j, k)
                );
                const std::array<std::size_t, 3> index{i, j, k};
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    if (std::abs(center[axis] -
                                 grid.cellCenters[axis][index[axis]]) >
                        grid.tolerance) {
                        throw std::invalid_argument(
                            "CSVExporter: cell centers are not on a Cartesian grid"
                        );
                    }
                }
            }
        }
    }

    for (std::size_t axis = 0; axis < 3; ++axis) {
        const auto& centers = grid.cellCenters[axis];
        if (!(grid.lower[axis] < centers.front()) ||
            !(centers.back() < grid.upper[axis])) {
            throw std::invalid_argument("CSVExporter: invalid mesh axis coordinates");
        }
        grid.sampleCoordinates[axis].clear();
        grid.sampleCoordinates[axis].push_back(grid.lower[axis]);
        grid.sampleCoordinates[axis].insert(
            grid.sampleCoordinates[axis].end(), centers.begin(), centers.end()
        );
        grid.sampleCoordinates[axis].push_back(grid.upper[axis]);
    }
    return grid;
}

std::size_t findCoordinateIndex(
    const std::vector<double>& coordinates,
    double value,
    double tolerance
)
{
    const auto found = std::lower_bound(coordinates.begin(), coordinates.end(), value);
    if (found != coordinates.end() && std::abs(*found - value) <= tolerance) {
        return static_cast<std::size_t>(found - coordinates.begin());
    }
    if (found != coordinates.begin()) {
        const auto previous = found - 1;
        if (std::abs(*previous - value) <= tolerance) {
            return static_cast<std::size_t>(previous - coordinates.begin());
        }
    }
    throw std::runtime_error("CSVExporter: boundary faces are not structured");
}

std::vector<BoundaryPlane> makeBoundaryPlanes(
    const Mesh& mesh,
    const ScalarField& field,
    double tolerance
)
{
    std::vector<BoundaryPlane> planes;
    planes.reserve(6);
    for (std::size_t patch = 0; patch < 6; ++patch) {
        const std::size_t axis = patch / 2;
        const std::size_t uAxis = axis == 0 ? 1 : 0;
        const std::size_t vAxis = axis == 2 ? 1 : 2;
        planes.push_back({uAxis, vAxis, {}, {}, {}});
    }

    for (std::size_t face = 0; face < mesh.faces.getNumber(); ++face) {
        if (!mesh.faces.isBoundary(face)) continue;
        const auto patch = mesh.faces.getBoundaryPatchId(face);
        if (patch >= planes.size()) {
            throw std::runtime_error("CSVExporter: boundary face has no patch ID");
        }
        const auto center = mesh.faces.getCenter(face);
        planes[patch].uCoordinates.push_back(center[planes[patch].uAxis]);
        planes[patch].vCoordinates.push_back(center[planes[patch].vAxis]);
    }

    for (auto& plane : planes) {
        const auto sortUnique = [tolerance](std::vector<double>& coordinates) {
            std::sort(coordinates.begin(), coordinates.end());
            std::vector<double> uniqueCoordinates;
            uniqueCoordinates.reserve(coordinates.size());
            for (const auto coordinate : coordinates) {
                if (uniqueCoordinates.empty() ||
                    std::abs(coordinate - uniqueCoordinates.back()) > tolerance) {
                    uniqueCoordinates.push_back(coordinate);
                }
            }
            coordinates.swap(uniqueCoordinates);
        };
        sortUnique(plane.uCoordinates);
        sortUnique(plane.vCoordinates);
        if (plane.uCoordinates.empty() || plane.vCoordinates.empty()) {
            throw std::runtime_error("CSVExporter: mesh is missing a boundary patch");
        }
        plane.values.assign(
            plane.uCoordinates.size() * plane.vCoordinates.size(),
            std::numeric_limits<double>::quiet_NaN()
        );
    }

    for (std::size_t face = 0; face < mesh.faces.getNumber(); ++face) {
        if (!mesh.faces.isBoundary(face)) continue;
        const auto patch = mesh.faces.getBoundaryPatchId(face);
        auto& plane = planes.at(patch);
        const auto center = mesh.faces.getCenter(face);
        const auto u = findCoordinateIndex(
            plane.uCoordinates, center[plane.uAxis], tolerance
        );
        const auto v = findCoordinateIndex(
            plane.vCoordinates, center[plane.vAxis], tolerance
        );
        const auto& condition = field.getBoundaryConditions().get(patch);
        const double faceValue = condition.type == BoundaryType::Dirichlet
            ? condition.value
            : field.getValue(mesh.faces.getOwner(face));
        plane.values[u * plane.vCoordinates.size() + v] = faceValue;
    }

    for (std::size_t patch = 0; patch < planes.size(); ++patch) {
        const auto& plane = planes[patch];
        if (std::any_of(plane.values.begin(), plane.values.end(),
                        [](double value) { return !std::isfinite(value); })) {
            throw std::runtime_error(
                "CSVExporter: boundary face values are incomplete for patch " +
                std::to_string(patch)
            );
        }
    }
    return planes;
}

class FieldInterpolator {
public:
    FieldInterpolator(
        const Mesh& mesh,
        const ScalarField& field,
        const StructuredGrid& grid
    )
        : mesh_(mesh),
          field_(field),
          grid_(grid),
          boundaryPlanes_(makeBoundaryPlanes(mesh, field, grid.tolerance))
    {
    }

    double sample(Point point) const
    {
        for (std::size_t axis = 0; axis < 3; ++axis) {
            if (point[axis] < grid_.lower[axis] - grid_.tolerance ||
                point[axis] > grid_.upper[axis] + grid_.tolerance) {
                throw std::out_of_range(
                    "CSVExporter: requested line lies outside the mesh"
                );
            }
            point[axis] = std::clamp(
                point[axis], grid_.lower[axis], grid_.upper[axis]
            );
        }

        std::array<Bracket, 3> brackets;
        for (std::size_t axis = 0; axis < 3; ++axis) {
            brackets[axis] = bracket(grid_.sampleCoordinates[axis], point[axis]);
        }

        double result = 0.0;
        for (std::size_t corner = 0; corner < 8; ++corner) {
            std::array<std::size_t, 3> index{};
            double weight = 1.0;
            for (std::size_t axis = 0; axis < 3; ++axis) {
                const bool high = ((corner >> axis) & 1U) != 0;
                index[axis] = high ? brackets[axis].upper : brackets[axis].lower;
                weight *= high ? brackets[axis].weight
                               : 1.0 - brackets[axis].weight;
            }
            if (weight != 0.0) result += weight * gridValue(index);
        }
        return result;
    }

private:
    double gridValue(const std::array<std::size_t, 3>& index) const
    {
        Point coordinates{};
        std::array<bool, 3> onBoundary{};
        std::array<std::size_t, 3> patchIds{};
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const auto& axisCoordinates = grid_.sampleCoordinates[axis];
            coordinates[axis] = axisCoordinates[index[axis]];
            if (index[axis] == 0 ||
                index[axis] + 1 == axisCoordinates.size()) {
                onBoundary[axis] = true;
                const bool high = index[axis] + 1 == axisCoordinates.size();
                patchIds[axis] = 2 * axis + (high ? 1 : 0);
            }
        }

        double boundarySum = 0.0;
        std::size_t boundaryCount = 0;
        for (std::size_t axis = 0; axis < 3; ++axis) {
            if (!onBoundary[axis]) continue;
            const auto& plane = boundaryPlanes_[patchIds[axis]];
            boundarySum += plane.valueAt(
                coordinates[plane.uAxis], coordinates[plane.vAxis]
            );
            ++boundaryCount;
        }
        if (boundaryCount != 0) {
            return boundarySum / static_cast<double>(boundaryCount);
        }

        return field_.getValue(mesh_.cells.getIdFromIndices(
            index[0] - 1, index[1] - 1, index[2] - 1
        ));
    }

    const Mesh& mesh_;
    const ScalarField& field_;
    const StructuredGrid& grid_;
    std::vector<BoundaryPlane> boundaryPlanes_;
};

Point interpolatePoint(const CSVLine& line, std::size_t pointIndex)
{
    const double t = line.resolution == 1
        ? 0.0
        : static_cast<double>(pointIndex) /
              static_cast<double>(line.resolution - 1);
    Point point{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        point[axis] = line.startPoint[axis] +
                      t * (line.endPoint[axis] - line.startPoint[axis]);
    }
    return point;
}

std::string csvHeader(const std::string& value)
{
    if (value.find_first_of(",\"\r\n") == std::string::npos) return value;
    std::string escaped = "\"";
    for (const char character : value) {
        if (character == '"') escaped += '"';
        escaped += character;
    }
    escaped += '"';
    return escaped;
}

void validate(
    const Mesh& mesh,
    const SimulationConfig& config,
    const OutputConfig& output
)
{
    std::set<std::string> names;
    for (const auto& field : config.scalarFields) {
        if (field.getName().empty() || !names.insert(field.getName()).second) {
            throw std::invalid_argument(
                "CSVExporter: field names must be nonempty and unique"
            );
        }
        if (field.getNumber() != mesh.cells.getNumber()) {
            throw std::invalid_argument(
                "CSVExporter: field size does not match mesh cell count"
            );
        }
    }
    for (const auto& line : output.csvLines) {
        if (line.resolution == 0) {
            throw std::invalid_argument("CSVExporter: line resolution must be positive");
        }
        for (const auto coordinate : line.startPoint) {
            if (!std::isfinite(coordinate)) {
                throw std::invalid_argument("CSVExporter: line coordinates must be finite");
            }
        }
        for (const auto coordinate : line.endPoint) {
            if (!std::isfinite(coordinate)) {
                throw std::invalid_argument("CSVExporter: line coordinates must be finite");
            }
        }
    }
}

} // namespace

void CSVExporter::write(
    const Mesh& mesh,
    const SimulationConfig& config,
    const OutputConfig& output,
    std::size_t timeStep
)
{
    if (!output.writeCsv) return;
    validate(mesh, config, output);
    if (output.csvLines.empty()) {
        throw std::invalid_argument("CSVExporter: csv=true but no lines are configured");
    }

    const auto grid = makeGrid(mesh);
    std::vector<FieldInterpolator> interpolators;
    interpolators.reserve(config.scalarFields.size());
    for (const auto& field : config.scalarFields) {
        interpolators.emplace_back(mesh, field, grid);
    }

    const auto suffix = timeStep == 0 ? std::string{} : "t" + std::to_string(timeStep);
    for (std::size_t lineIndex = 0; lineIndex < output.csvLines.size(); ++lineIndex) {
        const auto& line = output.csvLines[lineIndex];
        const auto filename =
            "line" + std::to_string(lineIndex + 1) + suffix + ".csv";
        std::ofstream file(filename);
        if (!file) {
            throw std::runtime_error("CSVExporter: cannot open file '" + filename + "'");
        }
        file.imbue(std::locale::classic());
        file << std::setprecision(std::numeric_limits<double>::max_digits10);
        file << "point";
        for (const auto& field : config.scalarFields) {
            file << ',' << csvHeader(field.getName());
        }
        file << '\n';

        for (std::size_t point = 0; point < line.resolution; ++point) {
            const auto coordinates = interpolatePoint(line, point);
            file << point + 1;
            for (const auto& interpolator : interpolators) {
                file << ',' << interpolator.sample(coordinates);
            }
            file << '\n';
        }
        file.close();
        if (!file) {
            throw std::runtime_error("CSVExporter: failed writing file '" + filename + "'");
        }
    }
}
