#pragma once

#include "Types.h"

namespace sim {

class FloorPanel {
public:
    explicit FloorPanel(int floor)
        : floor_(floor)
    {
    }

    int floor() const { return floor_; }

    HallCall press(int destination, int personId) const
    {
        return HallCall{personId, floor_, destination, -1};
    }

private:
    int floor_ = 1;
};

}  // namespace sim
