#pragma once

#include "Types.h"

#include <string>

namespace sim {

class Person {
public:
    Person(int id, int originFloor);

    int id() const { return id_; }
    int originFloor() const { return originFloor_; }
    int destinationFloor() const { return destinationFloor_; }
    PersonStatus status() const { return status_; }
    int assignedElevatorId() const { return assignedElevatorId_; }
    double waitSeconds() const { return waitSeconds_; }

    bool hasDestination() const { return destinationFloor_ > 0; }
    void setDestination(int floor);
    void startWaiting();
    void assignElevator(int elevatorId);
    void board(int elevatorId);
    void arrive();
    void evacuate();
    void tick(double dt);
    bool shouldRemove() const { return remove_; }
    std::string statusText() const;

private:
    int id_ = 0;
    int originFloor_ = 1;
    int destinationFloor_ = 0;
    PersonStatus status_ = PersonStatus::Created;
    int assignedElevatorId_ = -1;
    double waitSeconds_ = 0.0;
    double phaseSeconds_ = 0.0;
    bool remove_ = false;
};

}  // namespace sim
