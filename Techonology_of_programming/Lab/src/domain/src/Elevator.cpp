#include "Elevator.h"

#include "Constants.h"

#include <algorithm>
#include <cmath>

namespace sim {
namespace {

constexpr double kArrivePosEps = 0.03;
constexpr double kArriveVelEps = 0.05;

}  // namespace

Elevator::Elevator(ElevatorSpec spec, double floorHeight, int startFloor)
    : spec_(spec)
    , floorHeight_(floorHeight)
    , positionM_((startFloor - 1) * floorHeight)
{
}

int Elevator::currentFloor() const
{
    return static_cast<int>(std::lround(positionM_ / floorHeight_)) + 1;
}

int Elevator::targetFloor() const
{
    if (state_ == ElevatorState::FireRecall) {
        return 1;
    }
    return nextStop(state_ == ElevatorState::Moving);
}

bool Elevator::isStopped() const
{
    return std::abs(velocity_) < kArriveVelEps
        && (state_ == ElevatorState::Idle || state_ == ElevatorState::DoorsOpen
            || (state_ == ElevatorState::FireRecall && currentFloor() == 1 && doorsOpen_));
}

bool Elevator::atFloor(int floor) const
{
    return std::abs(positionM_ - floorPosition(floor)) < kArrivePosEps && std::abs(velocity_) < kArriveVelEps;
}

void Elevator::addStop(int floor)
{
    if (floor > 0) {
        stops_.insert(floor);
    }
}

void Elevator::board(int personId)
{
    if (!hasFreeSpace()) {
        return;
    }
    if (std::find(passengerIds_.begin(), passengerIds_.end(), personId) != passengerIds_.end()) {
        return;
    }
    passengerIds_.push_back(personId);
    tripWasEmpty_ = false;
}

void Elevator::alight(int personId)
{
    const auto before = passengerIds_.size();
    removePassenger(personId);
    if (passengerIds_.size() < before) {
        ++transportedCount_;
    }
}

void Elevator::removePassenger(int personId)
{
    passengerIds_.erase(std::remove(passengerIds_.begin(), passengerIds_.end(), personId), passengerIds_.end());
}

void Elevator::clearPassengers()
{
    passengerIds_.clear();
}

void Elevator::pressGo()
{
    goPressed_ = true;
}

void Elevator::offerDoors()
{
    if (state_ == ElevatorState::Idle && isStopped()) {
        openDoors();
    }
}

void Elevator::beginFireRecall()
{
    stops_.clear();
    goPressed_ = false;
    passengerIds_.clear();
    state_ = ElevatorState::FireRecall;
    if (atFloor(1)) {
        velocity_ = 0.0;
        positionM_ = floorPosition(1);
        setDirection(Direction::Idle);
        openDoors();
        doorTimer_ = 1e9;
        state_ = ElevatorState::FireRecall;
    } else {
        doorsOpen_ = false;
        doorTimer_ = 0.0;
        beginMovingTo(1);
        state_ = ElevatorState::FireRecall;
    }
}

void Elevator::endFireRecall()
{
    if (state_ != ElevatorState::FireRecall) {
        return;
    }
    stops_.clear();
    goPressed_ = false;
    if (atFloor(1)) {
        velocity_ = 0.0;
        positionM_ = floorPosition(1);
        setDirection(Direction::Idle);
        state_ = ElevatorState::Idle;
        doorsOpen_ = false;
        doorTimer_ = 0.0;
    } else {
        state_ = ElevatorState::Moving;
        addStop(1);
    }
}

void Elevator::tick(double dt)
{
    if (state_ == ElevatorState::FireRecall) {
        if (atFloor(1)) {
            velocity_ = 0.0;
            positionM_ = floorPosition(1);
            setDirection(Direction::Idle);
            doorsOpen_ = true;
            doorTimer_ = 1e9;
            return;
        }
        integrateToward(floorPosition(1), dt);
        if (atFloor(1)) {
            positionM_ = floorPosition(1);
            velocity_ = 0.0;
            setDirection(Direction::Idle);
            openDoors();
            doorTimer_ = 1e9;
            state_ = ElevatorState::FireRecall;
        }
        return;
    }

    if (state_ == ElevatorState::DoorsOpen) {
        doorTimer_ -= dt;
        if (doorTimer_ > 0.0) {
            return;
        }
        doorTimer_ = 0.0;
        const bool skipCurrent = !passengerIds_.empty() || goPressed_;
        int target = nextStop(skipCurrent);
        if (target <= 0) {
            target = nextStop(false);
        }
        if (target > 0 && !atFloor(target)) {
            doorsOpen_ = false;
            goPressed_ = false;
            beginMovingTo(target);
            return;
        }
        doorsOpen_ = false;
        goPressed_ = false;
        setDirection(Direction::Idle);
        state_ = ElevatorState::Idle;
        return;
    }

    if (state_ == ElevatorState::Idle) {
        if (stops_.empty()) {
            return;
        }
        const int target = nextStop();
        if (target <= 0) {
            return;
        }
        if (atFloor(target)) {
            stops_.erase(target);
            openDoors();
        } else {
            beginMovingTo(target);
        }
        return;
    }

    if (state_ == ElevatorState::Moving) {
        const int target = nextStop();
        if (target <= 0) {
            velocity_ = 0.0;
            setDirection(Direction::Idle);
            state_ = ElevatorState::Idle;
            return;
        }
        integrateToward(floorPosition(target), dt);
        if (atFloor(target)) {
            positionM_ = floorPosition(target);
            velocity_ = 0.0;
            stops_.erase(target);
            openDoors();
        }
    }
}

ElevatorStats Elevator::stats() const
{
    ElevatorStats row;
    row.elevatorId = spec_.id;
    row.trips = tripCount_;
    row.idleTrips = idleTripCount_;
    if (direction_ == Direction::Up || direction_ == Direction::Down) {
        ++row.trips;
        if (tripWasEmpty_) {
            ++row.idleTrips;
        }
    }
    row.idleTripPercent = row.trips > 0 ? 100.0 * row.idleTrips / static_cast<double>(row.trips) : 0.0;
    row.transported = transportedCount_;
    return row;
}

double Elevator::floorPosition(int floor) const
{
    return (floor - 1) * floorHeight_;
}

int Elevator::nextStop(bool skipCurrent) const
{
    if (stops_.empty()) {
        return 0;
    }
    const int here = currentFloor();

    auto firstUp = [&]() {
        int best = 0;
        for (int floor : stops_) {
            if (skipCurrent && floor == here) {
                continue;
            }
            if (floor >= here && (best == 0 || floor < best)) {
                best = floor;
            }
        }
        return best;
    };
    auto firstDown = [&]() {
        int best = 0;
        for (int floor : stops_) {
            if (skipCurrent && floor == here) {
                continue;
            }
            if (floor <= here && floor > best) {
                best = floor;
            }
        }
        return best;
    };

    if (direction_ == Direction::Down) {
        if (const int down = firstDown()) {
            return down;
        }
        return firstUp();
    }
    if (const int up = firstUp()) {
        return up;
    }
    return firstDown();
}

void Elevator::setDirection(Direction direction)
{
    if (direction == direction_) {
        return;
    }
    if (direction_ == Direction::Up || direction_ == Direction::Down) {
        ++tripCount_;
        if (tripWasEmpty_) {
            ++idleTripCount_;
        }
    }
    direction_ = direction;
    if (direction == Direction::Up || direction == Direction::Down) {
        tripWasEmpty_ = passengerIds_.empty();
    }
}

void Elevator::openDoors()
{
    state_ = ElevatorState::DoorsOpen;
    doorsOpen_ = true;
    doorTimer_ = kDoorSeconds;
    velocity_ = 0.0;
}

void Elevator::beginMovingTo(int floor)
{
    const int here = currentFloor();
    if (floor > here) {
        setDirection(Direction::Up);
    } else if (floor < here) {
        setDirection(Direction::Down);
    } else {
        setDirection(Direction::Idle);
        openDoors();
        return;
    }
    state_ = ElevatorState::Moving;
    doorsOpen_ = false;
}

void Elevator::integrateToward(double targetPos, double dt)
{
    const double dist = targetPos - positionM_;
    const double absDist = std::abs(dist);
    if (absDist < 1e-4 && std::abs(velocity_) < 1e-3) {
        positionM_ = targetPos;
        velocity_ = 0.0;
        return;
    }

    const double sign = dist >= 0.0 ? 1.0 : -1.0;
    const double stopSpeed = std::sqrt(std::max(0.0, 2.0 * spec_.maxAcceleration * absDist));
    const double desired = sign * std::min(spec_.maxSpeed, stopSpeed);
    const double dv = desired - velocity_;
    const double maxDv = spec_.maxAcceleration * dt;
    if (std::abs(dv) <= maxDv) {
        velocity_ = desired;
    } else {
        velocity_ += (dv > 0.0 ? maxDv : -maxDv);
    }

    positionM_ += velocity_ * dt;
    const double newDist = targetPos - positionM_;
    if (newDist * dist < 0.0) {
        positionM_ = targetPos;
        velocity_ = 0.0;
    }
}

}  // namespace sim
