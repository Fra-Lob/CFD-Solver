#include "TestUtils.hpp"

#include <cfd/mesh/MeshBuilder.hpp>
#include <cfd/meshio/MeshGenerator.hpp>
#include <cfd/numerics/LinearSystem.hpp>
#include <cfd/physics/BoundaryConditions.hpp>
#include <cfd/process/Diffusion.hpp>

#include <eigen3/Eigen/IterativeLinearSolvers>
#include <eigen3/Eigen/Sparse>

namespace test {

void testDiffusion()
{
    constexpr double lengthX = 1.0;
    constexpr double valueAtXMin = 100.0;
    constexpr double valueAtXMax = 0.0;
    constexpr double diffusivity = 1.0;
    constexpr std::size_t nx = 10;

    auto mesh = cfd::MeshGenerator::generate({lengthX, 1.0, 1.0, nx, 2, 2});
    cfd::MeshBuilder::computeConnectivity(mesh);
    cfd::MeshBuilder::computeFullGeometry(mesh);
    cfd::MeshBuilder::computeBoundaryPatch(mesh);

    cfd::BoundaryConditions boundaryConditions;
    boundaryConditions.set(
        static_cast<std::size_t>(cfd::CartesianPatch::XMin),
        cfd::BoundaryType::Dirichlet,
        valueAtXMin
    );
    boundaryConditions.set(
        static_cast<std::size_t>(cfd::CartesianPatch::XMax),
        cfd::BoundaryType::Dirichlet,
        valueAtXMax
    );
    for (const auto patch : {
             cfd::CartesianPatch::YMin, cfd::CartesianPatch::YMax,
             cfd::CartesianPatch::ZMin, cfd::CartesianPatch::ZMax}) {
        boundaryConditions.set(
            static_cast<std::size_t>(patch),
            cfd::BoundaryType::Neumann,
            0.0
        );
    }

    cfd::LinearSystem system(mesh.cells.getNumber());
    cfd::Diffusion::assemble(mesh, diffusivity, boundaryConditions, system);
    system.finalize();

    Eigen::ConjugateGradient<
        Eigen::SparseMatrix<double>,
        Eigen::Lower | Eigen::Upper
    > solver;
    solver.setTolerance(1e-12);
    solver.compute(system.getMatrix());
    require(solver.info() == Eigen::Success, "Diffusion solver setup");

    const Eigen::VectorXd solution = solver.solve(system.getRhs());
    require(solver.info() == Eigen::Success, "Diffusion linear solve");
    require(solution.size() == static_cast<Eigen::Index>(mesh.cells.getNumber()),
            "Diffusion solution size");
    require(solution.allFinite(), "Diffusion solution values are finite");

    for (std::size_t cell = 0; cell < mesh.cells.getNumber(); ++cell) {
        const auto center = mesh.cells.getCenter(cell);
        const double expected =
            valueAtXMin +
            (valueAtXMax - valueAtXMin) * center[0] / lengthX;
        require(approx(solution[static_cast<Eigen::Index>(cell)], expected, 1e-9),
                "Diffusion solution follows the analytical linear x profile");
    }
}

}
