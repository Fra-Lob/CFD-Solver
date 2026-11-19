#pragma once

#include <cfd/mesh/Mesh.hpp>

namespace cfd {

    class MeshConnectivityBuilder {
        // ===================================================================================== //
        //                               Connectivity construction                               //
        // ===================================================================================== //
    public:
        static void build(Mesh& mesh);
    };

} // namespace cfd
