#include <cassert>
#include <process/Convection.hpp> 
#include <stdexcept>

void Convection::assemble(const Mesh&               mesh,
                          const VectorField&        velocity,
                          const double              rho,
                          const BoundaryConditions& bc,
                                LinearSystem&       system) {

    std::size_t nFaces = mesh.faces.getNumber();
    for (std::size_t faceId = 0; faceId < nFaces; faceId++) {

        auto ownerId = mesh.faces.getOwner(faceId);

        // ------------------------------------------------------ //
        //                    Internal Cells                      //
        // ------------------------------------------------------ //
        if (!mesh.faces.isBoundary(faceId)) {

            auto neighbourId = mesh.faces.getNeighbor(faceId);

            // Velocity vector on the face
            double velocityX = (velocity.getValueX(ownerId) + 
                                velocity.getValueX(neighbourId)) / 2;
            double velocityY = (velocity.getValueY(ownerId) + 
                                velocity.getValueY(neighbourId)) / 2;
            double velocityZ = (velocity.getValueZ(ownerId) + 
                                velocity.getValueZ(neighbourId)) / 2;

            // Scalar product for signed flux
            double coefficient = math::scalarProduct({velocityX, velocityY, velocityZ},
                                                     mesh.faces.getAreaVector(faceId));
            coefficient *= rho;

            // Assembling
            system.addCoefficient(ownerId,     ownerId,     // A_PP  
                                  std::max(coefficient, 0.0));   
            system.addCoefficient(ownerId,     neighbourId, // A_PN
                                  std::min(coefficient, 0.0)); 
            system.addCoefficient(neighbourId, neighbourId, // A_NN
                                  - std::min(coefficient, 0.0));   
            system.addCoefficient(neighbourId, ownerId,     // A_NP
                                  - std::max(coefficient, 0.0)); 
        }
        // ------------------------------------------------------ //
        //                    Boundary Cells                      //
        // ------------------------------------------------------ //
        else {

            auto condition = bc.get(mesh.faces.getBoundaryPatchId(faceId));

            // Velocity vector on the face
            double velocityX = velocity.getValueX(ownerId);
            double velocityY = velocity.getValueY(ownerId);
            double velocityZ = velocity.getValueZ(ownerId);

            // Scalar product for signed flux
            double coefficient = math::scalarProduct({velocityX, velocityY, velocityZ},
                                                     mesh.faces.getAreaVector(faceId));
            coefficient *= rho;

            // Inflow, only dirichtler
            if (coefficient < 0) {
                if (condition.type != BoundaryType::Dirichlet) {
                    throw std::runtime_error("Only Dirichlet conditions for inflow");
                }
                system.addRHS(ownerId, - coefficient * condition.value);
            }

            // Outflow, we use anyway internal cell value
            else {
                system.addCoefficient(ownerId, ownerId, coefficient);
            }
        }
    }
}