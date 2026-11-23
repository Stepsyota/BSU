#include "Scenario.h"

#include <algorithm>

namespace sim {

Scenario::Scenario(std::vector<ScenarioEvent> events)
{
    setEvents(std::move(events));
}

void Scenario::setEvents(std::vector<ScenarioEvent> events)
{
    events_ = std::move(events);
    std::sort(events_.begin(), events_.end(), [](const ScenarioEvent& a, const ScenarioEvent& b) {
        return a.timeSeconds < b.timeSeconds;
    });
    next_ = 0;
}

void Scenario::reset()
{
    next_ = 0;
}

std::vector<ScenarioEvent> Scenario::dueEvents(double now)
{
    std::vector<ScenarioEvent> due;
    while (next_ < events_.size() && events_[next_].timeSeconds <= now) {
        due.push_back(events_[next_]);
        ++next_;
    }
    return due;
}

}  // namespace sim
