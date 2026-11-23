#pragma once

#include "Constants.h"

#include <vector>

namespace sim {

struct ElevatorSpec {
    int id = 1;
    int capacity = kDefaultCapacity;
    double maxSpeed = kDefaultMaxSpeed;
    double maxAcceleration = kDefaultMaxAccel;
};

struct BuildingConfig {
    int floorCount = kDefaultFloorCount;
    double floorHeight = kFloorHeightM;
    std::vector<ElevatorSpec> elevators;

    static BuildingConfig defaults()
    {
        BuildingConfig config;
        config.floorCount = kDefaultFloorCount;
        config.floorHeight = kFloorHeightM;
        config.elevators.clear();
        for (int i = 1; i <= kDefaultElevatorCount; ++i) {
            config.elevators.push_back(ElevatorSpec{i, kDefaultCapacity, kDefaultMaxSpeed, kDefaultMaxAccel});
        }
        return config;
    }

    bool isValid() const
    {
        if (floorCount < kMinFloors || floorCount > kMaxFloors) {
            return false;
        }
        if (floorHeight <= 0.0) {
            return false;
        }
        const int n = static_cast<int>(elevators.size());
        if (n < kMinElevators || n > kMaxElevators) {
            return false;
        }
        for (const auto& spec : elevators) {
            if (spec.id <= 0 || spec.capacity <= 0 || spec.maxSpeed <= 0.0 || spec.maxAcceleration <= 0.0) {
                return false;
            }
        }
        return true;
    }
};

}  // namespace sim
