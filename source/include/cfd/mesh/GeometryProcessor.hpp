#pragma once

#include <cfd/mesh/Mesh.hpp>

namespace cfd {

    class MeshGeometryProcessor {
        // ===================================================================================== //
        //                                 Geometry calculations                                 //
        // ===================================================================================== //
    public:
        static void validateCurvedMetrics(const Mesh& mesh);
        static void validateCurvedGrid(const Mesh& mesh);
        static void computeFaces(Mesh& mesh);
        static void computeCells(Mesh& mesh);

        static void compute(Mesh& mesh) {
            computeFaces(mesh);
            computeCells(mesh);
        }
    };

} // namespace cfd