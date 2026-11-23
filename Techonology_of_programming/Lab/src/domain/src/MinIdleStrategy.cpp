#include "MinIdleStrategy.h"

#include "Elevator.h"

#include <cmath>
#include <limits>

namespace sim {

int MinIdleStrategy::assign(const HallCall& call, const std::vector<Elevator>& elevators, double floorHeight) const
{
    int bestId = -1;
    double bestScore = -std::numeric_limits<double>::infinity();
    const Direction callDir = call.destinationFloor > call.originFloor ? Direction::Up : Direction::Down;

    for (const auto& elevator : elevators) {
        if (elevator.state() == ElevatorState::FireRecall) {
            continue;
        }
        double score = 0.0;
        if (!elevator.hasFreeSpace()) {
            score -= 1000.0;
        }
        if (!elevator.isEmpty() && (elevator.direction() == callDir || elevator.direction() == Direction::Idle)) {
            score += 100.0;
        }
        if (elevator.direction() == callDir) {
            score += 50.0;
        }
        if (elevator.direction() == Direction::Idle) {
            score += 10.0;
        }
        if (elevator.isEmpty() && elevator.direction() != Direction::Idle) {
            score -= 40.0;
        }
        const double dist = std::abs(elevator.positionM() - (call.originFloor - 1) * floorHeight);
        score -= dist;
        if (score > bestScore) {
            bestScore = score;
            bestId = elevator.id();
        }
    }
    return bestId;
}

}  // namespace sim
