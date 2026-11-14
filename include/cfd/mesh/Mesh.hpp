#pragma once

#include <cfd/utils/Math.hpp>
#include <cfd/mesh/Nodes.hpp>
#include <cfd/mesh/Faces.hpp>
#include <cfd/mesh/Cells.hpp>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace cfd {

    // Values are also used by the VTU metadata format; keep them stable.
    enum class MeshType { Unknown = 0, Cartesian = 1, Cylindrical = 2, Spherical = 3 };

    class Mesh {
    
    private:
        MeshType type_ = MeshType::Unknown;
        unsigned dimension_ = 0;
        std::vector<double> domain_;
        
    public:
        Nodes nodes;
        Faces faces;
        Cells cells;

        // A default mesh has unspecified properties (e.g. an older VTU file).
        Mesh() = default;

        Mesh(MeshType type, unsigned dimension, const std::vector<double>& domain) {
            setProperties(type, dimension, domain);
        }

        // Parameter order (angles in radians):
        // Cartesian:   {Lx}, {Lx, Ly}, {Lx, Ly, Lz}
        // Cylindrical: {R}, {L, R}, {L, R, theta}
        // Spherical:   {R}, {R, theta}, {R, theta, phi}
        // Domain starts at zero in every coordinate; changing metadata does not
        // move nodes or rebuild boundary patches. Set all properties atomically.
        void setProperties(MeshType type, unsigned dimension, const std::vector<double>& domain) {
            if (type != MeshType::Cartesian && type != MeshType::Cylindrical && type != MeshType::Spherical)
                throw std::invalid_argument("Mesh: a known coordinate type is required");
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

        void requireCartesian3D() const {
            if (type_ != MeshType::Cartesian || dimension_ != 3)
                throw std::invalid_argument("This operation currently supports only Cartesian 3D meshes with domain properties");
        }
    };

}
