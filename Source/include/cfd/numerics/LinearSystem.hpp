#pragma once

#include <eigen3/Eigen/Sparse>
#include <eigen3/Eigen/IterativeLinearSolvers>
#include <eigen3/Eigen/src/Core/Matrix.h>
#include <vector>

class LinearSystem {
private:
    std::size_t size_;

    std::vector<Eigen::Triplet<double>> triplets_;

    Eigen::SparseMatrix<double> A_;
    Eigen::VectorXd b_;

public:
    explicit LinearSystem(std::size_t size):
        size_(size),
        A_(size, size),
        b_(Eigen::VectorXd::Zero(size)) {
            triplets_.reserve(7 * size);
        }

    void addCoefficient(std::size_t row, std::size_t col, double value) {
        triplets_.emplace_back(row, col, value);
    }

    void addRHS(std::size_t row, double value) {
        b_(row) += value;
    }

    void finalize() {
        A_.setFromTriplets(triplets_.begin(), triplets_.end());
        A_.makeCompressed();
        triplets_.clear();
    }

    void reset() {
        triplets_.clear();
        A_.setZero();
        b_.setZero();
    }

    const Eigen::SparseMatrix<double>& getMatrix() const {
        return A_;
    }
    const Eigen::VectorXd& getRhs() const {
        return b_;
    }
};