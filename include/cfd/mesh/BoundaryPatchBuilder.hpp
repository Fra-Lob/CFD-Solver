#pragma once

#include <cfd/mesh/Mesh.hpp>

namespace cfd {

class MeshBoundaryPatchBuilder {
public:
    static void build(Mesh& mesh);
    static void buildLogical(Mesh& mesh);
    // Classify boundary faces on [0,Lx] x [0,Ly] x [0,Lz] using mesh properties.
    // IDs: xmin=0, xmax=1, ymin=2, ymax=3, zmin=4, zmax=5.
    // Connectivity must already be built; throws if a face cannot be classified.
    static void buildCartesian(
        Mesh& mesh
    );
};

}
