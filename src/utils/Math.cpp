#include <cfd/utils/Math.hpp>
#include <cmath>
#include <array>

namespace cfd::math {

    Vector3 opposite(const Vector3& a) {
        return{-a[0], -a[1], -a[2]};
    }

    Vector3 vectorSum(const Vector3& a, const Vector3& b,
                      const Vector3& c, const Vector3& d) {
        return {
            a[0] + b[0] + c[0] + d[0],
            a[1] + b[1] + c[1] + d[1],
            a[2] + b[2] + c[2] + d[2]};
    }

    Vector3 productByCoefficient(const Vector3& a, const double k) {
        return {a[0] * k, a[1] * k, a[2] * k};
    }

    Vector3 divisionByCoefficient(const Vector3& a, const double k) {
        return {a[0] / k, a[1] / k, a[2] / k};
    }

    Vector3 vectorProduct(const Vector3& a, const Vector3& b) {
        return {
            a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]
        };
    }

    double scalarProduct(const Vector3& a, const Vector3& b) {
        return a[0] * b[0] +
               a[1] * b[1] +
               a[2] * b[2];
    }

    double norm(const Vector3& a) {
        return std::sqrt(scalarProduct(a, a));
    }
}