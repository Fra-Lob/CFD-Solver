#include <cfd/meshio/GridConfigReader.hpp>

#include <charconv>
#include <fstream>
#include <locale>
#include <set>
#include <sstream>

namespace cfd {
    namespace {
        // ===================================================================================== //
        //                                 Text parsing helpers                                  //
        // ===================================================================================== //
        std::string trim(const std::string& text) {
            const auto first = text.find_first_not_of(" \t\r\n");
            return first == std::string::npos
                       ? ""
                       : text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
        }
    } // namespace

    // ===================================================================================== //
    //                         Configuration loading and validation                          //
    // ===================================================================================== //
    MeshConfig GridConfigReader::read(const std::filesystem::path& directory) {
        const auto path = directory / "grid.info";
        std::ifstream file(path);
        if (!file) {
            throw std::runtime_error("Cannot open " + path.string());
        }
        std::map<std::string, std::string> fields;
        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(file, line)) {
            ++lineNumber;
            line = line.substr(0, line.find_first_of("#;"));
            line = trim(line);
            if (line.empty()) {
                continue;
            }
            const auto assignmentPosition = line.find('=');
            if (assignmentPosition == std::string::npos ||
                line.find('=', assignmentPosition + 1) != std::string::npos) {
                throw std::runtime_error("grid.info line " + std::to_string(lineNumber) +
                                         ": expected key = value");
            }
            const auto key = trim(line.substr(0, assignmentPosition)),
                       value = trim(line.substr(assignmentPosition + 1));
            if (key.empty() || value.empty() || !fields.emplace(key, value).second) {
                throw std::runtime_error("grid.info: empty or duplicate field at line " +
                                         std::to_string(lineNumber));
            }
        }
        if (file.bad()) {
            throw std::runtime_error("Error reading " + path.string());
        }
        std::set<std::string> usedFields;
        const auto get = [&](const std::string& key) {
            usedFields.insert(key);
            if (!fields.contains(key)) {
                throw std::runtime_error("grid.info: missing " + key);
            }
            return fields.at(key);
        };
        const auto real = [&](const std::string& key) {
            std::istringstream stream(get(key));
            stream.imbue(std::locale::classic());
            double value;
            if (!(stream >> value) || stream.peek() != std::char_traits<char>::eof() ||
                !std::isfinite(value)) {
                throw std::runtime_error("grid.info: invalid real number for " + key);
            }
            return value;
        };
        const auto count = [&](const std::string& key) {
            const auto text = get(key);
            std::size_t value = 0;
            const auto [end, error] =
                std::from_chars(text.data(), text.data() + text.size(), value);
            if (error != std::errc{} || end != text.data() + text.size() || value == 0) {
                throw std::runtime_error("grid.info: invalid positive integer for " + key);
            }
            return value;
        };
        MeshConfig config;
        config.name = get("name");
        if (config.name == "." || config.name == ".." ||
            config.name.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.") !=
                std::string::npos) {
            throw std::runtime_error("grid.info: name must be a plain file stem");
        }
        const auto type = get("type");
        const double degreesToRadians = std::numbers::pi / 180.0;
        if (type == "cartesian") {
            config.type = MeshType::Cartesian;
            config.domain = {real("xMin"), real("xMax"), real("yMin"),
                             real("yMax"), real("zMin"), real("zMax")};
            config.resolution[0] = count("Nx");
            config.resolution[1] = count("Ny");
            config.resolution[2] = count("Nz");
        } else if (type == "cylindrical") {
            config.type = MeshType::Cylindrical;
            config.domain = {real("rInner"),
                             real("rOuter"),
                             degreesToRadians * real("thetaStart"),
                             degreesToRadians * real("thetaEnd"),
                             real("zMin"),
                             real("zMax")};
            config.resolution[0] = count("Nr");
            config.resolution[1] = count("Ntheta");
            config.resolution[2] = count("Nz");
        } else if (type == "spherical") {
            config.type = MeshType::Spherical;
            config.domain = {real("rInner"),
                             real("rOuter"),
                             degreesToRadians * real("polarStart"),
                             degreesToRadians * real("polarEnd"),
                             degreesToRadians * real("azimuthStart"),
                             degreesToRadians * real("azimuthEnd")};
            config.resolution[0] = count("Nr");
            config.resolution[1] = count("Ntheta");
            config.resolution[2] = count("Nphi");
        } else if (type == "custom") {
            config.type = MeshType::Custom;
        } else {
            throw std::runtime_error("grid.info: unsupported type " + type);
        }
        for (const auto& [key, value] : fields) {
            if (!usedFields.contains(key)) {
                throw std::runtime_error("grid.info: unknown field " + key);
            }
        }
        if (config.type != MeshType::Custom) {
            MeshGenerator::validate(config);
        }
        return config;
    }
} // namespace cfd
