#pragma once

#include "IDispatchStrategy.h"

namespace sim {

class MinWaitStrategy : public IDispatchStrategy {
public:
    std::string name() const override { return "Минимум времени ожидания"; }
    int assign(const HallCall& call, const std::vector<Elevator>& elevators, double floorHeight) const override;
};

}  // namespace sim
