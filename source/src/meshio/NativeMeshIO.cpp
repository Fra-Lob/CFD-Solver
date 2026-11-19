#include <cfd/mesh/MeshValidator.hpp>
#include <cfd/meshio/MeshReader.hpp>
#include <cfd/meshio/MeshWriter.hpp>

#include <bit>
#include <cstdint>
#include <fstream>
#include <limits>

namespace cfd {
    namespace {
        // ===================================================================================== //
        //                           Native format and stream helpers                            //
        // ===================================================================================== //
        constexpr char fileIdentifier[8] = {'C', 'F', 'D', 'M', 'E', 'S', 'H', '\0'};
        constexpr std::uint32_t formatVersion = 2;
        constexpr std::uint32_t metricConvention = 2;
        constexpr std::size_t maximumStringBytes = 1024 * 1024;
        constexpr std::uint64_t fnvOffsetBasis = 14695981039346656037ull;
        constexpr std::uint64_t fnvPrime = 1099511628211ull;
        constexpr unsigned nodeRecordBytes = 24;
        constexpr unsigned cellRecordBytes = 144;
        constexpr unsigned faceRecordBytes = 112;
        static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559);

        // ===================================================================================== //
        //                             Writing portable binary values                            //
        // ===================================================================================== //
        class BinaryOutput {
            std::ofstream file;
            std::uint64_t checksum_ = fnvOffsetBasis;

        public:
            explicit BinaryOutput(const std::filesystem::path& path)
                : file(path, std::ios::binary | std::ios::trunc) {
                if (!file) {
                    throw std::runtime_error("Cannot write " + path.string());
                }
            }

            void bytes(const char* data, std::size_t size) {
                file.write(data, static_cast<std::streamsize>(size));
                for (std::size_t i = 0; i < size; ++i) {
                    checksum_ ^= static_cast<unsigned char>(data[i]);
                    checksum_ *= fnvPrime;
                }
            }

            void integer(std::uint64_t value, unsigned size = 8) {
                for (unsigned i = 0; i < size; ++i) {
                    const char b = static_cast<char>((value >> (8 * i)) & 255);
                    bytes(&b, 1);
                }
            }

            void real(double value) {
                integer(std::bit_cast<std::uint64_t>(value));
            }

            void id(std::size_t value) {
                integer(value == constants::INVALID_ID ? UINT64_MAX : value);
            }

            void vector(const math::Vector3& v) {
                for (double value : v) {
                    real(value);
                }
            }

            void string(const std::string& s) {
                if (s.size() > maximumStringBytes) {
                    throw std::runtime_error("Native mesh string exceeds 1 MiB");
                }
                integer(s.size());
                bytes(s.data(), s.size());
            }

            void close() {
                const auto digest = checksum_;
                integer(digest);
                file.close();
                if (!file) {
                    throw std::runtime_error("Native mesh write failed");
                }
            }
        };

        // ===================================================================================== //
        //                             Reading portable binary values                            //
        // ===================================================================================== //
        class BinaryInput {
            std::ifstream file;
            std::uint64_t remaining_;
            std::uint64_t checksum_ = fnvOffsetBasis;

        public:
            explicit BinaryInput(const std::filesystem::path& path)
                : file(path, std::ios::binary), remaining_(0) {
                if (!file) {
                    throw std::runtime_error("Cannot read " + path.string());
                }
                remaining_ = std::filesystem::file_size(path);
            }

            std::uint64_t remaining() const {
                return remaining_;
            }

            std::uint64_t checksum() const {
                return checksum_;
            }

            void bytes(char* data, std::size_t n) {
                if (n > remaining_) {
                    throw std::runtime_error("Truncated native mesh");
                }
                file.read(data, static_cast<std::streamsize>(n));
                if (!file) {
                    throw std::runtime_error("Native mesh read failed");
                }
                remaining_ -= n;
                for (std::size_t i = 0; i < n; ++i) {
                    checksum_ ^= static_cast<unsigned char>(data[i]);
                    checksum_ *= fnvPrime;
                }
            }

            std::uint64_t integer(unsigned size = 8) {
                unsigned char b[8]{};
                bytes(reinterpret_cast<char*>(b), size);
                std::uint64_t v = 0;
                for (unsigned i = 0; i < size; ++i) {
                    v |= std::uint64_t(b[i]) << (8 * i);
                }
                return v;
            }

            std::size_t count() {
                const auto v = integer();
                if (v >= std::numeric_limits<std::size_t>::max()) {
                    throw std::runtime_error("Native mesh count overflow");
                }
                return static_cast<std::size_t>(v);
            }

            std::size_t id() {
                const auto v = integer();
                if (v == UINT64_MAX) {
                    return constants::INVALID_ID;
                }
                if (v >= std::numeric_limits<std::size_t>::max()) {
                    throw std::runtime_error("Native mesh ID overflow");
                }
                return static_cast<std::size_t>(v);
            }

            double real() {
                const auto v = std::bit_cast<double>(integer());
                if (!std::isfinite(v)) {
                    throw std::runtime_error("Nonfinite native mesh value");
                }
                return v;
            }

            math::Vector3 vector() {
                math::Vector3 v;
                for (auto& x : v) {
                    x = real();
                }
                return v;
            }

            std::string string() {
                const auto n = count();
                if (n > maximumStringBytes || n > remaining_) {
                    throw std::runtime_error("Invalid native mesh string length");
                }
                std::string s(n, '\0');
                bytes(s.data(), n);
                return s;
            }
        };
    } // namespace

    // ===================================================================================== //
    //                               Writing a completed mesh                                //
    // ===================================================================================== //
    void MeshWriter::write(const Mesh& mesh, const std::filesystem::path& filename) {
        MeshValidator::validate(mesh);
        BinaryOutput output(filename);
        // File header and domain identification.
        output.bytes(fileIdentifier, 8);
        output.integer(formatVersion, 4);
        output.integer(metricConvention, 4);
        output.integer(static_cast<unsigned>(mesh.getType()), 4);
        output.string(mesh.getName());
        for (auto d : mesh.getDomain()) {
            output.real(d);
        }

        for (auto count : mesh.getResolution()) {
            output.integer(count);
        }

        // Boundary names are metadata; per-face IDs are stored with the faces.
        output.integer(mesh.getPatchNames().size());
        for (const auto& [tag, name] : mesh.getPatchNames()) {
            output.id(tag);
            output.string(name);
        }

        // Logical dimensions and entity counts.
        for (auto n : {mesh.nodes.getNx(), mesh.nodes.getNy(), mesh.nodes.getNz(),
                       mesh.cells.getNx(), mesh.cells.getNy(), mesh.cells.getNz()}) {
            output.integer(n);
        }
        output.integer(mesh.nodes.getNumber());
        output.integer(mesh.cells.getNumber());
        output.integer(mesh.faces.getNumber());

        // Node coordinates.
        for (std::size_t n = 0; n < mesh.nodes.getNumber(); ++n) {
            output.vector(mesh.nodes.getCoordinates(n));
        }

        // Cell connectivity, volumes and centers.
        for (std::size_t c = 0; c < mesh.cells.getNumber(); ++c) {
            for (auto n : mesh.cells.getNodes(c)) {
                output.id(n);
            }
            for (auto f : mesh.cells.getFaces(c)) {
                output.id(f);
            }
            output.real(mesh.cells.getVolume(c));
            output.vector(mesh.cells.getCenter(c));
        }

        // Face connectivity, geometry and boundary classification.
        for (std::size_t f = 0; f < mesh.faces.getNumber(); ++f) {
            for (auto n : mesh.faces.getNodes(f)) {
                output.id(n);
            }
            output.id(mesh.faces.getOwner(f));
            output.id(mesh.faces.getNeighbor(f));
            output.vector(mesh.faces.getCenter(f));
            output.real(mesh.faces.getArea(f));
            output.vector(mesh.faces.getAreaVector(f));
            output.id(mesh.faces.getBoundaryPatchId(f));
        }

        // Complete the file with its checksum and verify the stream state.
        output.close();
    }

    // ===================================================================================== //
    //                               Restoring a completed mesh                              //
    // ===================================================================================== //
    Mesh MeshReader::read(const std::filesystem::path& filename) {
        try {
            BinaryInput input(filename);
            char header[8];
            input.bytes(header, 8);
            if (!std::equal(std::begin(header), std::end(header), std::begin(fileIdentifier))) {
                throw std::runtime_error("Invalid native mesh identifier");
            }
            if (input.integer(4) != formatVersion || input.integer(4) != metricConvention) {
                throw std::runtime_error("Unsupported native mesh version or metric convention");
            }
            const auto type = input.integer(4);
            if (type > 4) {
                throw std::runtime_error("Invalid mesh type");
            }
            Mesh mesh;
            mesh.setName(input.string());
            MeshDomain domain;
            for (auto& bound : domain) {
                bound = input.real();
            }
            MeshResolution resolution;
            for (auto& count : resolution) {
                count = input.count();
            }
            if (type == 0) {
                if (domain != MeshDomain{} || resolution != MeshResolution{}) {
                    throw std::runtime_error("Inconsistent unspecified mesh properties");
                }
            } else {
                mesh.setProperties(static_cast<MeshType>(type), domain, resolution);
            }
            const auto numberOfPatches = input.count();
            if (numberOfPatches > input.remaining() / 16) {
                throw std::runtime_error("Invalid patch count");
            }
            std::map<std::size_t, std::string> patches;
            for (std::size_t p = 0; p < numberOfPatches; ++p) {
                const auto id = input.id();
                const auto name = input.string();
                if (!patches.emplace(id, name).second) {
                    throw std::runtime_error("Duplicate patch ID");
                }
            }
            mesh.setPatchNames(patches);
            std::array<std::size_t, 6> logicalDimensions;
            for (auto& d : logicalDimensions) {
                d = input.count();
            }
            mesh.nodes.setDimensions(logicalDimensions[0], logicalDimensions[1],
                                     logicalDimensions[2]);
            mesh.cells.setDimensions(logicalDimensions[3], logicalDimensions[4],
                                     logicalDimensions[5]);
            const auto numberOfNodes = input.count();
            const auto numberOfCells = input.count();
            const auto numberOfFaces = input.count();
            // Validate record sizes against the actual file before allocating entities.
            if (input.remaining() < 8) {
                throw std::runtime_error("Missing native mesh checksum");
            }
            auto available = input.remaining() - 8;
            for (auto [count, size] : {std::pair{numberOfNodes, nodeRecordBytes},
                                       std::pair{numberOfCells, cellRecordBytes},
                                       std::pair{numberOfFaces, faceRecordBytes}}) {
                if (count > available / size) {
                    throw std::runtime_error("Native mesh counts exceed file size");
                }
                available -= count * size;
            }
            if (available != 0) {
                throw std::runtime_error("Native mesh has trailing data");
            }
            for (std::size_t n = 0; n < numberOfNodes; ++n) {
                const auto p = input.vector();
                mesh.nodes.addCoordinates(p[0], p[1], p[2]);
            }
            for (std::size_t c = 0; c < numberOfCells; ++c) {
                std::array<std::size_t, 8> nodes;
                for (auto& n : nodes) {
                    n = input.id();
                    if (n >= numberOfNodes) {
                        throw std::runtime_error("Invalid cell node ID");
                    }
                }
                std::array<std::size_t, 6> faces;
                for (auto& f : faces) {
                    f = input.id();
                    if (f >= numberOfFaces) {
                        throw std::runtime_error("Invalid cell face ID");
                    }
                }
                mesh.cells.addByNodes(nodes, c);
                mesh.cells.setFaces(c, faces);
                mesh.cells.setVolume(c, input.real());
                mesh.cells.setCenter(c, input.vector());
            }
            for (std::size_t f = 0; f < numberOfFaces; ++f) {
                std::array<std::size_t, 4> nodes;
                for (auto& n : nodes) {
                    n = input.id();
                    if (n >= numberOfNodes) {
                        throw std::runtime_error("Invalid face node ID");
                    }
                }
                const auto owner = input.id(), neighbor = input.id();
                if (owner >= numberOfCells ||
                    (neighbor != constants::INVALID_ID && neighbor >= numberOfCells)) {
                    throw std::runtime_error("Invalid face adjacency");
                }
                mesh.faces.addByNodes(nodes, owner, neighbor);
                mesh.faces.setCenter(f, input.vector());
                mesh.faces.setArea(f, input.real());
                mesh.faces.setAreaVector(f, input.vector());
                mesh.faces.setBoundaryPatchId(f, input.id());
            }
            const auto digest = input.checksum();
            if (input.integer() != digest) {
                throw std::runtime_error("Native mesh checksum mismatch");
            }
            MeshValidator::validate(mesh);
            return mesh;
        } catch (const std::exception& e) {
            throw std::runtime_error("Cannot read native mesh '" + filename.string() +
                                     "': " + e.what());
        }
    }
} // namespace cfd
