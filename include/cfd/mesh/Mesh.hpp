#pragma once

#include <cfd/utils/Math.hpp>
#include <cfd/mesh/Nodes.hpp>
#include <cfd/mesh/Faces.hpp>
#include <cfd/mesh/Cells.hpp>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <string>
#include <map>
#include <numbers>

namespace cfd {

    // Values are also used by the VTU metadata format; keep them stable.
    enum class MeshType { Unknown = 0, Cartesian = 1, Cylindrical = 2, Spherical = 3, Custom = 4 };

    class Mesh {
    
    private:
        MeshType type_ = MeshType::Unknown;
        unsigned dimension_ = 0;
        std::vector<double> domain_;
        std::string name_ = "mesh";
        std::map<std::size_t, std::string> patches_;
        
    public:
        Nodes nodes;
        Faces faces;
        Cells cells;

        // A default mesh has unspecified properties (e.g. an older VTU file).
        Mesh() = default;

        Mesh(MeshType type, unsigned dimension, const std::vector<double>& domain) {
            setProperties(type, dimension, domain);
        }

        // Legacy short descriptor order (angles in radians):
        // Cartesian:   {Lx}, {Lx, Ly}, {Lx, Ly, Lz}
        // Cylindrical: {R}, {L, R}, {L, R, theta}
        // Spherical:   {R}, {R, theta}, {R, theta, phi}
        // Full 3D sectors: cylindrical {ri,ro,L,theta0,theta1}, spherical
        // {ri,ro,polar0,polar1,azimuth0,azimuth1}; Custom uses an empty domain.
        // Changing metadata does not move nodes or rebuild boundary patches.
        // Set all properties atomically.
        void setProperties(MeshType type, unsigned dimension, const std::vector<double>& domain) {
            if (type != MeshType::Cartesian && type != MeshType::Cylindrical && type != MeshType::Spherical && type != MeshType::Custom)
                throw std::invalid_argument("Mesh: a known coordinate type is required");
            const bool sector = dimension == 3 &&
                ((type == MeshType::Cylindrical && domain.size() == 5) ||
                 (type == MeshType::Spherical && domain.size() == 6));
            const bool custom = type == MeshType::Custom && dimension == 3 && domain.empty();
            if (sector || custom) {
                for (double value : domain) if (!std::isfinite(value))
                    throw std::invalid_argument("Mesh: domain parameters must be finite");
                if (sector) {
                    if (domain[0] <= 0 || domain[1] <= domain[0])
                        throw std::invalid_argument("Mesh: require 0 < rInner < rOuter");
                    if (type == MeshType::Cylindrical) {
                        if (domain[2] <= 0 || domain[4] <= domain[3] || domain[4] - domain[3] >= 2*std::numbers::pi)
                            throw std::invalid_argument("Mesh: invalid cylindrical length or nonperiodic angular sector");
                    } else if (domain[2] <= 0 || domain[3] >= std::numbers::pi || domain[3] <= domain[2] ||
                               domain[5] <= domain[4] || domain[5] - domain[4] >= 2*std::numbers::pi) {
                        throw std::invalid_argument("Mesh: invalid spherical sector (poles and periodic topology unsupported)");
                    }
                }
                domain_ = domain; type_ = type; dimension_ = dimension;
                return;
            }
            if (type == MeshType::Custom) throw std::invalid_argument("Mesh: custom meshes require dimension 3 and an empty domain");
            if (dimension < 1 || dimension > 3 || domain.size() != dimension)
                throw std::invalid_argument("Mesh: dimension must be 1, 2 or 3 with one domain parameter per dimension");
            for (const double value : domain) {
                if (!std::isfinite(value) || value <= 0.0)
                    throw std::invalid_argument("Mesh: domain parameters must be finite and positive");
            }
            constexpr double pi = 3.14159265358979323846;
            if (type == MeshType::Cylindrical && dimension == 3 && domain[2] > 2 * pi)
                throw std::invalid_argument("Mesh: cylindrical theta must not exceed 2*pi");
            if (type == MeshType::Spherical && dimension >= 2 && domain[1] > pi)
                throw std::invalid_argument("Mesh: spherical theta must not exceed pi");
            if (type == MeshType::Spherical && dimension == 3 && domain[2] > 2 * pi)
                throw std::invalid_argument("Mesh: spherical phi must not exceed 2*pi");
            domain_ = domain;
            type_ = type;
            dimension_ = dimension;
        }

        MeshType getType() const { return type_; }
        unsigned getDimension() const { return dimension_; }
        const std::vector<double>& getDomain() const { return domain_; }
        bool hasProperties() const { return type_ != MeshType::Unknown; }

        void setName(const std::string& name) {
            if (name.empty()) throw std::invalid_argument("Mesh name cannot be empty");
            name_ = name;
        }
        const std::string& getName() const { return name_; }
        void setPatchNames(const std::map<std::size_t, std::string>& patches) {
            for (const auto& [id, name] : patches)
                if (id == constants::INVALID_ID || name.empty()) throw std::invalid_argument("Invalid patch metadata");
            patches_ = patches;
        }
        const std::map<std::size_t, std::string>& getPatchNames() const { return patches_; }

        void requireHex3D() const {
            if (hasProperties() && dimension_ != 3)
                throw std::invalid_argument("Only three-dimensional hexahedral meshes are implemented");
        }

        void requireCartesian3D() const {
            if (type_ != MeshType::Cartesian || dimension_ != 3)
                throw std::invalid_argument("This operation currently supports only Cartesian 3D meshes with domain properties");
        }
    };

}
