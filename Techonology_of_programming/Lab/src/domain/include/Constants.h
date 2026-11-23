#pragma once

namespace sim {

constexpr double kFloorHeightM = 3.0;
constexpr double kTickSeconds = 0.05;
constexpr double kDoorSeconds = 1.0;
constexpr double kPersonTimeoutSeconds = 3.0;
constexpr int kMinFloors = 1;
constexpr int kMaxFloors = 20;
constexpr int kMinElevators = 1;
constexpr int kMaxElevators = 5;
constexpr double kMinSpeedFactor = 0.25;
constexpr double kMaxSpeedFactor = 8.0;
constexpr int kDefaultCapacity = 8;
constexpr double kDefaultMaxSpeed = 2.0;
constexpr double kDefaultMaxAccel = 1.0;
constexpr int kDefaultFloorCount = 10;
constexpr int kDefaultElevatorCount = 3;
constexpr int kMaxLivingPeople = 80;

}  // namespace sim
