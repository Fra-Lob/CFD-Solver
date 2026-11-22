#pragma once

#include <physics/BoundaryConditions.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class ScalarField {
private:
    std::string name_;
    std::vector<double> values_;
    BoundaryCondition bc_;
    double diffusivity_ = 0.0;
    double initialValue_ = 0.0;
    BoundaryConditions boundaryConditions_;

public:

// ================================================================================//
//                                  Constructor                                    //
// ================================================================================//

    ScalarField(std::string name, std::size_t nValues, double initialValue): 
        name_(std::move(name)),
        values_(nValues, initialValue),
        initialValue_(initialValue) {}

// ================================================================================//
//                                      Getters                                    //
// ================================================================================//

    const std::string& getName() const {
        return name_;
    }

    std::size_t getNumber() const {
        return values_.size();
    }

    double getValue(std::size_t cellId) const {
        return values_.at(cellId);
    }

    const BoundaryCondition& getBoundaryCondition() const {
        return bc_;
    }

    double getDiffusivity() const {
        return diffusivity_;
    }

    double getInitialValue() const {
        return initialValue_;
    }

    const BoundaryConditions& getBoundaryConditions() const {
        return boundaryConditions_;
    }

// ================================================================================//
//                                      Setters                                    //
// ================================================================================//

    void setValue(std::size_t cellId, double value) {
        values_.at(cellId) = value;
    }

    void fill(double value) {
        std::fill(values_.begin(), values_.end(), value);
    }

    void setBoundaryCondition(BoundaryCondition& bc) {
        bc_ = bc;
    }

    void setDiffusivity(double diffusivity) {
        diffusivity_ = diffusivity;
    }

    void setBoundaryConditions(const BoundaryConditions& boundaryConditions) {
        boundaryConditions_ = boundaryConditions;
    }
};