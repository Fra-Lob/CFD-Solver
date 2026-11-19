#pragma once

#include <cfd/mesh/Cells.hpp>
#include <cfd/mesh/Faces.hpp>
#include <cfd/mesh/Nodes.hpp>
#include <cfd/utils/Math.hpp>

#include <array>
#include <limits>
#include <map>
#include <numbers>
#include <string>

namespace cfd {

    // Values are also used by the VTU metadata format; keep them stable.
    enum class MeshType {
        Unknown = 0,
        Cartesian = 1,
        Cylindrical = 2,
        Spherical = 3,
        Custom = 4
    };

    using MeshDomain = std::array<double, 6>;
    using MeshResolution = std::array<std::size_t, 3>;

    class Mesh {

    private:
        MeshType type_ = MeshType::Unknown;
        MeshDomain domain_{};
        MeshResolution resolution_{};
        std::string name_ = "mesh";
        std::map<std::size_t, std::string> patches_;

    public:
        // ===================================================================================== //
        //                                   Entity containers                                   //
        // ===================================================================================== //
        Nodes nodes;
        Faces faces;
        Cells cells;

        // A default mesh has unspecified properties (e.g. an older VTU file).
        // ===================================================================================== //
        //                                     Construction                                      //
        // ===================================================================================== //
        Mesh() = default;

        Mesh(MeshType type,
             const MeshDomain& domain,
             const MeshResolution& resolution,
             const std::string& name = "mesh") {
            setProperties(type, domain, resolution);
            setName(name);
        }

        // ===================================================================================== //
        //                                   Domain properties                                   //
        // ===================================================================================== //
        // Three coordinate intervals: {firstIn, firstOut, secondIn, secondOut, thirdIn, thirdOut}.
        // Cartesian: x/y/z; cylindrical: r/theta/z; spherical: r/polar/azimuth.
        // Angles are radians. Imported unstructured meshes use zero resolution.
        // This setter changes metadata only; it does not regenerate existing entities.
        void
        setProperties(MeshType type, const MeshDomain& domain, const MeshResolution& resolution) {
            if (type != MeshType::Cartesian && type != MeshType::Cylindrical &&
                type != MeshType::Spherical && type != MeshType::Custom) {
                throw std::invalid_argument("Mesh: a known coordinate type is required");
            }
            for (double value : domain) {
                if (!std::isfinite(value)) {
                    throw std::invalid_argument("Mesh: domain bounds must be finite");
                }
            }
            if (type != MeshType::Custom) {
                for (unsigned axis = 0; axis < 3; ++axis) {
                    if (domain[2 * axis + 1] <= domain[2 * axis]) {
                        throw std::invalid_argument("Mesh: each coordinate interval must increase");
                    }
                    if (resolution[axis] <= 1 ||
                        resolution[axis] == std::numeric_limits<std::size_t>::max()) {
                        throw std::invalid_argument(
                            "Mesh: current 3D generation requires every resolution > 1");
                    }
                }
                if (type != MeshType::Cartesian && domain[0] <= 0) {
                    throw std::invalid_argument(
                        "Mesh: rInner must be positive (axis/origin unsupported)");
                }
                if (type == MeshType::Cylindrical && domain[3] - domain[2] > 2 * std::numbers::pi) {
                    throw std::invalid_argument(
                        "Mesh: cylindrical angular extent cannot exceed a full turn");
                }
                if (type == MeshType::Spherical &&
                    (domain[2] <= 0 || domain[3] >= std::numbers::pi ||
                     domain[5] - domain[4] > 2 * std::numbers::pi)) {
                    throw std::invalid_argument("Mesh: spherical poles and angular extents beyond "
                                                "a full turn are unsupported");
                }
                const unsigned angularAxis = type == MeshType::Cylindrical ? 1 : 2;
                if (type != MeshType::Cartesian &&
                    (domain[2 * angularAxis + 1] - domain[2 * angularAxis]) /
                            resolution[angularAxis] >=
                        std::numbers::pi) {
                    throw std::invalid_argument(
                        "Mesh: each angular cell must span less than 180 degrees");
                }
            } else if (resolution != MeshResolution{}) {
                throw std::invalid_argument("Mesh: unstructured meshes have no logical resolution");
            }
            type_ = type;
            domain_ = domain;
            resolution_ = resolution;
        }

        // Physical position at normalized domain coordinates, each in [0, 1].
        math::Vector3 point(const math::Vector3& fraction) const {
            math::Vector3 coordinate;
            for (unsigned axis = 0; axis < 3; ++axis) {
                coordinate[axis] = domain_[2 * axis] +
                                   fraction[axis] * (domain_[2 * axis + 1] - domain_[2 * axis]);
            }
            const auto [a, b, c] = coordinate;
            switch (type_) {
            case MeshType::Cartesian:
                return coordinate;
            case MeshType::Cylindrical:
                return {a * std::cos(b), a * std::sin(b), c};
            case MeshType::Spherical:
                return {a * std::sin(b) * std::cos(c), a * std::sin(b) * std::sin(c),
                        a * std::cos(b)};
            default:
                throw std::invalid_argument(
                    "Mesh: unstructured coordinates require explicit nodes");
            }
        }

        // ===================================================================================== //
        //                                   Property accessors                                  //
        // ===================================================================================== //
        MeshType getType() const {
            return type_;
        }

        const MeshDomain& getDomain() const {
            return domain_;
        }

        const MeshResolution& getResolution() const {
            return resolution_;
        }

        bool isPeriodic(unsigned axis) const {
            const bool angularAxis = (type_ == MeshType::Cylindrical && axis == 1) ||
                                     (type_ == MeshType::Spherical && axis == 2);
            return angularAxis && domain_[2 * axis + 1] - domain_[2 * axis] == 2 * std::numbers::pi;
        }

        MeshResolution nodeResolution() const {
            MeshResolution result;
            for (unsigned axis = 0; axis < 3; ++axis) {
                result[axis] = resolution_[axis] + (isPeriodic(axis) ? 0 : 1);
            }
            return result;
        }

        bool isCurved() const {
            return type_ == MeshType::Cylindrical || type_ == MeshType::Spherical;
        }

        bool hasProperties() const {
            return type_ != MeshType::Unknown;
        }

        // ===================================================================================== //
        //                             Mesh and patch identification                             //
        // ===================================================================================== //
        void setName(const std::string& name) {
            if (name.empty()) {
                throw std::invalid_argument("Mesh name cannot be empty");
            }
            name_ = name;
        }

        const std::string& getName() const {
            return name_;
        }

        void setPatchNames(const std::map<std::size_t, std::string>& patches) {
            for (const auto& [id, name] : patches) {
                if (id == constants::INVALID_ID || name.empty()) {
                    throw std::invalid_argument("Invalid patch metadata");
                }
            }
            patches_ = patches;
        }

        const std::map<std::size_t, std::string>& getPatchNames() const {
            return patches_;
        }

        // ===================================================================================== //
        //                               Supported operation checks                              //
        // ===================================================================================== //
        void requireCartesian3D() const {
            if (type_ != MeshType::Cartesian) {
                throw std::invalid_argument("This operation requires Cartesian domain properties");
            }
        }
    };

} // namespace cfd
