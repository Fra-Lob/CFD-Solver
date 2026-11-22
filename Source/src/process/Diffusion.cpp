#include "cfd/physics/BoundaryConditions.hpp"
#include <cfd/process/Diffusion.hpp>
#include <cstddef>

void cfd::Diffusion::assemble(const cfd::Mesh& mesh,
                    double gamma,
              const cfd::BoundaryConditions& bc,
                    cfd::LinearSystem& system) {
    
    std::size_t nFaces = mesh.faces.getNumber();
    for (std::size_t faceId = 0; faceId < nFaces; faceId++) {

        auto ownerId = mesh.faces.getOwner(faceId);
        auto ownerCenter = mesh.cells.getCenter(ownerId);

        // ------------------------------------------------------ //
        //                    Internal Cells                      //
        // ------------------------------------------------------ //
        if (!mesh.faces.isBoundary(faceId)) {

            auto neighbourId = mesh.faces.getNeighbor(faceId);
            auto neighbourCenter = mesh.cells.getCenter(neighbourId);
            
            double distance = cfd::math::norm(cfd::math::vectorSum(
                ownerCenter,
                cfd::math::opposite(neighbourCenter)
            ));
            double coefficient = gamma * mesh.faces.getArea(faceId) / distance;

            system.addCoefficient(ownerId,     ownerId,     coefficient);   // A_PP
            system.addCoefficient(ownerId,     neighbourId, - coefficient); // A_PN
            system.addCoefficient(neighbourId, neighbourId, coefficient);   // A_NN
            system.addCoefficient(neighbourId, ownerId,     - coefficient); // A_NP
        }
        // ------------------------------------------------------ //
        //                    Boundary Cells                      //
        // ------------------------------------------------------ //
        else {
            auto faceCenter = mesh.faces.getCenter(faceId);
            double distance = cfd::math::norm(cfd::math::vectorSum(
                ownerCenter,
                cfd::math::opposite(faceCenter)
            ));

            auto condition = bc.get(mesh.faces.getBoundaryPatchId(faceId));

            // Dirichlet Conditions
            if (condition.type == cfd::BoundaryType::Dirichlet) {
                double coefficient = gamma * mesh.faces.getArea(faceId) / distance;

                system.addCoefficient(ownerId, ownerId, coefficient);
                system.addRHS(ownerId, coefficient * condition.value);
            }

            // Neumann Conditions
            if (condition.type == cfd::BoundaryType::Neumann) {
                double coefficient = gamma * mesh.faces.getArea(faceId);

                system.addRHS(ownerId, coefficient * condition.value);
            }
        }   
    }
}