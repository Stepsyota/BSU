#pragma once

#include "IDispatchStrategy.h"

namespace sim {

class MinIdleStrategy : public IDispatchStrategy {
public:
    std::string name() const override { return "Минимум холостых ходов"; }
    int assign(const HallCall& call, const std::vector<Elevator>& elevators, double floorHeight) const override;
};

}  // namespace sim
