#include "MinWaitStrategy.h"

#include "Constants.h"
#include "Elevator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace sim {

int MinWaitStrategy::assign(const HallCall& call, const std::vector<Elevator>& elevators, double floorHeight) const
{
    int bestId = -1;
    double bestEta = std::numeric_limits<double>::infinity();

    for (const auto& elevator : elevators) {
        if (elevator.state() == ElevatorState::FireRecall) {
            continue;
        }
        const double originPos = (call.originFloor - 1) * floorHeight;
        const double dist = std::abs(elevator.positionM() - originPos);
        const double speed = std::max(elevator.maxSpeed(), 0.1);
        double eta = dist / speed + elevator.stopCount() * kDoorSeconds;
        if (!elevator.hasFreeSpace()) {
            eta += 30.0;
        }
        if (elevator.direction() == Direction::Up && call.originFloor < elevator.currentFloor()) {
            eta += 8.0;
        }
        if (elevator.direction() == Direction::Down && call.originFloor > elevator.currentFloor()) {
            eta += 8.0;
        }
        if (eta < bestEta) {
            bestEta = eta;
            bestId = elevator.id();
        }
    }
    return bestId;
}

}  // namespace sim
