#pragma once

#include <array>

namespace cfd::math {

    using Vector3 = std::array<double, 3>;

    Vector3 opposite(const Vector3& a);

    Vector3 vectorSum(const Vector3& a, const Vector3& b,
                      const Vector3& c = {0, 0, 0}, const Vector3& d = {0, 0, 0});

    Vector3 productByCoefficient(const Vector3& a, const double k);

    Vector3 divisionByCoefficient(const Vector3& a, const double k);

    Vector3 vectorProduct(const Vector3& a, const Vector3& b);

    double scalarProduct(const Vector3& a, const Vector3& b);

    double norm(const Vector3& a);

}