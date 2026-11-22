#include "TestUtils.hpp"

#include <cfd/numerics/LinearSystem.hpp>

namespace test {

    void testLinearSystem() {

        LinearSystem system(3);
        system.addCoefficient(0, 0, 4.0);
        system.addCoefficient(0, 1, -1.0);
        system.addCoefficient(0, 1, -2.0);
        system.addCoefficient(1, 0, 2.0);
        system.addCoefficient(2, 2, 5.0);
        system.addRHS(0, 1.0);
        system.addRHS(0, 2.0);
        system.addRHS(1, -4.0);
        system.finalize();

        const auto& matrix = system.getMatrix();
        const auto& rhs = system.getRhs();
        require(matrix.rows() == 3 && matrix.cols() == 3, "Linear system dimensions");
        require(approx(matrix.coeff(0, 0), 4.0), "Linear system diagonal coefficient");
        require(approx(matrix.coeff(0, 1), -3.0), "Duplicate coefficients must be summed");
        require(approx(matrix.coeff(1, 0), 2.0), "Linear system off-diagonal coefficient");
        require(approx(matrix.coeff(2, 2), 5.0), "Linear system final diagonal coefficient");
        require(approx(rhs(0), 3.0) && approx(rhs(1), -4.0) && approx(rhs(2), 0.0),
                "Linear system right-hand side accumulation");

        system.reset();
        require(system.getMatrix().nonZeros() == 0, "Linear system reset matrix");
        require(system.getRhs().isZero(0.0), "Linear system reset right-hand side");
    }
}