#pragma once

#include <vector>

namespace sim {

enum class ScenarioEventType { SpawnPeople, FireOn, FireOff };

struct ScenarioEvent {
    double timeSeconds = 0.0;
    ScenarioEventType type = ScenarioEventType::SpawnPeople;
    int count = 0;
    int fromFloor = 1;
    int toFloor = 1;
};

class Scenario {
public:
    Scenario() = default;
    explicit Scenario(std::vector<ScenarioEvent> events);

    const std::vector<ScenarioEvent>& events() const { return events_; }
    void setEvents(std::vector<ScenarioEvent> events);
    void reset();
    std::vector<ScenarioEvent> dueEvents(double now);

private:
    std::vector<ScenarioEvent> events_;
    std::size_t next_ = 0;
};

}  // namespace sim
