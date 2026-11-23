#pragma once

namespace sim {

enum class Direction { Idle, Up, Down };
enum class PersonStatus { Created, Waiting, Moving, Delivered, Evacuating };
enum class ElevatorState { Idle, Moving, DoorsOpen, FireRecall };
enum class SessionState { Stopped, Running, Paused };

struct HallCall {
    int personId = 0;
    int originFloor = 1;
    int destinationFloor = 1;
    int assignedElevatorId = -1;
};

}  // namespace sim
