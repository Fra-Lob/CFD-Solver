#pragma once

#include <cstddef>
#include <unordered_map>

enum class BoundaryType {
    Dirichlet,
    Neumann
};

struct BoundaryCondition {
public:
    BoundaryType type = BoundaryType::Neumann;
    double value = 0.0;
};

class BoundaryConditions {
private:
    std::unordered_map<std::size_t, BoundaryCondition> conditions_;

public:
    void set(std::size_t patchId, BoundaryType type, double value) {
        conditions_[patchId] = {type, value};
    }

    const BoundaryCondition& get(std::size_t patchId) const {
        return conditions_.at(patchId);
    }
};