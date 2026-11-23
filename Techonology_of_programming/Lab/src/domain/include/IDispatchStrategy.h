#pragma once

#include "Types.h"

#include <string>
#include <vector>

namespace sim {

class Elevator;

class IDispatchStrategy {
public:
    virtual ~IDispatchStrategy() = default;
    virtual std::string name() const = 0;
    virtual int assign(const HallCall& call, const std::vector<Elevator>& elevators, double floorHeight) const = 0;
};

}  // namespace sim
