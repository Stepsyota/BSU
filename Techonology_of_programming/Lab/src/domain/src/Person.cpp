#include "Person.h"

#include "Constants.h"

#include <iomanip>
#include <sstream>

namespace sim {

Person::Person(int id, int originFloor)
    : id_(id)
    , originFloor_(originFloor)
{
}

void Person::setDestination(int floor)
{
    destinationFloor_ = floor;
}

void Person::startWaiting()
{
    status_ = PersonStatus::Waiting;
    phaseSeconds_ = 0.0;
}

void Person::assignElevator(int elevatorId)
{
    assignedElevatorId_ = elevatorId;
}

void Person::board(int elevatorId)
{
    assignedElevatorId_ = elevatorId;
    status_ = PersonStatus::Moving;
}

void Person::arrive()
{
    status_ = PersonStatus::Delivered;
    assignedElevatorId_ = -1;
    phaseSeconds_ = 0.0;
}

void Person::evacuate()
{
    status_ = PersonStatus::Evacuating;
    assignedElevatorId_ = -1;
    phaseSeconds_ = 0.0;
}

void Person::tick(double dt)
{
    if (status_ == PersonStatus::Waiting) {
        waitSeconds_ += dt;
    }

    if (status_ == PersonStatus::Created || status_ == PersonStatus::Delivered
        || status_ == PersonStatus::Evacuating) {
        phaseSeconds_ += dt;
        if (phaseSeconds_ >= kPersonTimeoutSeconds) {
            remove_ = true;
        }
    }
}

std::string Person::statusText() const
{
    std::ostringstream out;
    switch (status_) {
    case PersonStatus::Created:
        out << "набирает этаж назначения";
        break;
    case PersonStatus::Waiting:
        out << std::fixed << std::setprecision(1) << "ожидает лифт " << waitSeconds_ << " с";
        break;
    case PersonStatus::Moving:
        out << "движется на этаж " << destinationFloor_;
        break;
    case PersonStatus::Delivered:
        out << "доставлен на целевой этаж";
        break;
    case PersonStatus::Evacuating:
        out << "эвакуация по пожарной лестнице";
        break;
    }
    return out.str();
}

}  // namespace sim
