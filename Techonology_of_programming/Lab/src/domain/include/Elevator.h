#pragma once

#include "BuildingConfig.h"
#include "Statistics.h"
#include "Types.h"

#include <set>
#include <vector>

namespace sim {

class Elevator {
public:
    Elevator(ElevatorSpec spec, double floorHeight, int startFloor = 1);

    int id() const { return spec_.id; }
    int capacity() const { return spec_.capacity; }
    double maxSpeed() const { return spec_.maxSpeed; }
    double maxAcceleration() const { return spec_.maxAcceleration; }
    double positionM() const { return positionM_; }
    double velocity() const { return velocity_; }
    Direction direction() const { return direction_; }
    ElevatorState state() const { return state_; }
    bool doorsOpen() const { return doorsOpen_; }
    const std::vector<int>& passengerIds() const { return passengerIds_; }
    const std::set<int>& stops() const { return stops_; }
    int stopCount() const { return static_cast<int>(stops_.size()); }
    int passengerCount() const { return static_cast<int>(passengerIds_.size()); }

    int currentFloor() const;
    int targetFloor() const;
    bool isEmpty() const { return passengerIds_.empty(); }
    bool isStopped() const;
    bool hasFreeSpace() const { return passengerCount() < spec_.capacity; }
    bool atFloor(int floor) const;

    void addStop(int floor);
    void board(int personId);
    void alight(int personId);
    void removePassenger(int personId);
    void clearPassengers();
    void pressGo();
    void offerDoors();
    void beginFireRecall();
    void endFireRecall();
    void tick(double dt);

    ElevatorStats stats() const;

private:
    double floorPosition(int floor) const;
    int nextStop(bool skipCurrent = false) const;
    void setDirection(Direction direction);
    void openDoors();
    void beginMovingTo(int floor);
    void integrateToward(double targetPos, double dt);

    ElevatorSpec spec_;
    double floorHeight_ = kFloorHeightM;
    double positionM_ = 0.0;
    double velocity_ = 0.0;
    Direction direction_ = Direction::Idle;
    ElevatorState state_ = ElevatorState::Idle;
    bool doorsOpen_ = false;
    double doorTimer_ = 0.0;
    bool goPressed_ = false;
    std::vector<int> passengerIds_;
    std::set<int> stops_;

    int tripCount_ = 0;
    int idleTripCount_ = 0;
    int transportedCount_ = 0;
    bool tripWasEmpty_ = true;
};

}  // namespace sim
