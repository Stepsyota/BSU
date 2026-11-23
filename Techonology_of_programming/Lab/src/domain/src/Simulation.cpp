#include "Simulation.h"

#include "MinIdleStrategy.h"
#include "MinWaitStrategy.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

namespace sim {
namespace {

std::string formatStops(const std::set<int>& stops)
{
    std::ostringstream out;
    bool first = true;
    for (int floor : stops) {
        if (!first) {
            out << ", ";
        }
        first = false;
        out << floor;
    }
    return out.str();
}

}  // namespace

Simulation::Simulation()
{
    strategy_ = std::make_unique<MinWaitStrategy>();
    rebuildElevators();
}

bool Simulation::configure(BuildingConfig config)
{
    if (state_ != SessionState::Stopped || !config.isValid()) {
        return false;
    }
    config_ = std::move(config);
    people_.clear();
    pendingCalls_.clear();
    nextPersonId_ = 1;
    rebuildElevators();
    resetSessionCounters();
    fireAlarm_ = false;
    timeSeconds_ = 0.0;
    return true;
}

void Simulation::setStrategy(std::unique_ptr<IDispatchStrategy> strategy)
{
    if (state_ != SessionState::Stopped || !strategy) {
        return;
    }
    strategy_ = std::move(strategy);
}

bool Simulation::start()
{
    if (state_ != SessionState::Stopped) {
        return false;
    }
    if (!strategy_) {
        strategy_ = std::make_unique<MinWaitStrategy>();
    }

    people_.erase(std::remove_if(people_.begin(), people_.end(),
                                 [](const Person& person) {
                                     return person.status() == PersonStatus::Delivered
                                         || person.status() == PersonStatus::Evacuating;
                                 }),
                  people_.end());
    for (auto& person : people_) {
        person.assignElevator(-1);
    }
    for (auto& call : pendingCalls_) {
        call.assignedElevatorId = -1;
    }

    rebuildElevators();
    resetSessionCounters();
    scenario_.reset();
    fireAlarm_ = false;
    timeSeconds_ = 0.0;
    state_ = SessionState::Running;
    return true;
}

void Simulation::pause()
{
    if (state_ == SessionState::Running) {
        state_ = SessionState::Paused;
    }
}

void Simulation::resume()
{
    if (state_ == SessionState::Paused) {
        state_ = SessionState::Running;
    }
}

bool Simulation::tryStop()
{
    if (state_ == SessionState::Stopped) {
        return true;
    }
    if (!canStop()) {
        return false;
    }
    fireAlarm_ = false;
    state_ = SessionState::Stopped;
    return true;
}

bool Simulation::canStop() const
{
    return std::all_of(elevators_.begin(), elevators_.end(), [](const Elevator& elevator) {
        return elevator.isEmpty() && elevator.isStopped();
    });
}

void Simulation::setSpeedFactor(double factor)
{
    speedFactor_ = std::clamp(factor, kMinSpeedFactor, kMaxSpeedFactor);
}

void Simulation::setFireAlarm(bool on)
{
    if (state_ == SessionState::Stopped) {
        return;
    }
    if (on && !fireAlarm_) {
        ++fireCount_;
        fireAlarm_ = true;
        evacuateOccupants();
        pendingCalls_.clear();
        for (auto& elevator : elevators_) {
            elevator.beginFireRecall();
        }
        return;
    }
    if (!on && fireAlarm_) {
        fireAlarm_ = false;
        for (auto& elevator : elevators_) {
            elevator.endFireRecall();
        }
    }
}

void Simulation::setScenario(Scenario scenario)
{
    scenario_ = std::move(scenario);
    if (state_ == SessionState::Stopped) {
        scenario_.reset();
    }
}

int Simulation::spawnPerson(int originFloor, int destinationFloor)
{
    if (static_cast<int>(people_.size()) >= kMaxLivingPeople) {
        return -1;
    }
    if (originFloor < 1 || originFloor > config_.floorCount) {
        return -1;
    }
    if (destinationFloor != 0 && destinationFloor == originFloor) {
        return -1;
    }
    Person person(nextPersonId_++, originFloor);
    const int id = person.id();
    people_.push_back(std::move(person));

    if (fireAlarm_) {
        if (auto* spawned = personById(id)) {
            spawned->evacuate();
        }
        return id;
    }
    if (destinationFloor > 0) {
        submitDestination(id, destinationFloor);
    }
    return id;
}

bool Simulation::submitDestination(int personId, int destFloor)
{
    if (fireAlarm_) {
        return false;
    }
    Person* person = personById(personId);
    if (!person || person->status() != PersonStatus::Created || person->hasDestination()) {
        return false;
    }
    if (destFloor < 1 || destFloor > config_.floorCount || destFloor == person->originFloor()) {
        return false;
    }
    const FloorPanel* panel = panelByFloor(person->originFloor());
    if (!panel) {
        return false;
    }
    pendingCalls_.push_back(panel->press(destFloor, personId));
    person->setDestination(destFloor);
    person->startWaiting();
    return true;
}

void Simulation::tick(double realDt)
{
    if (state_ != SessionState::Running) {
        return;
    }
    const double dt = realDt * speedFactor_;
    if (dt <= 0.0) {
        return;
    }
    timeSeconds_ += dt;
    applyScenario(timeSeconds_);
    tickPeople(dt);

    if (fireAlarm_) {
        fireDuration_ += dt;
        evacuateOccupants();
        pendingCalls_.clear();
        for (auto& elevator : elevators_) {
            if (elevator.state() != ElevatorState::FireRecall) {
                elevator.beginFireRecall();
            }
            elevator.tick(dt);
        }
        removeExpiredPeople();
        return;
    }

    assignCalls();
    handleStopsAndPassengers();
    for (auto& elevator : elevators_) {
        elevator.tick(dt);
    }
    removeExpiredPeople();
}

SessionSnapshot Simulation::snapshot() const
{
    SessionSnapshot shot;
    shot.state = state_;
    shot.timeSeconds = timeSeconds_;
    shot.fireAlarm = fireAlarm_;
    shot.canStop = canStop();
    shot.strategyName = strategy_ ? strategy_->name() : std::string{};
    shot.speedFactor = speedFactor_;
    shot.floorCount = config_.floorCount;
    shot.floorHeight = config_.floorHeight;

    int transported = 0;
    shot.elevators.reserve(elevators_.size());
    for (const auto& elevator : elevators_) {
        const ElevatorStats stats = elevator.stats();
        transported += stats.transported;
        ElevatorRow row;
        row.id = elevator.id();
        row.floor = elevator.currentFloor();
        row.positionM = elevator.positionM();
        row.velocity = elevator.velocity();
        row.state = elevator.state();
        row.doorsOpen = elevator.doorsOpen();
        row.passengers = elevator.passengerCount();
        row.capacity = elevator.capacity();
        row.nextStop = elevator.targetFloor();
        row.stops = formatStops(elevator.stops());
        row.trips = stats.trips;
        row.idlePercent = stats.idleTripPercent;
        shot.elevators.push_back(std::move(row));
    }
    shot.transported = transported;

    shot.people.reserve(people_.size());
    for (const auto& person : people_) {
        PersonRow row;
        row.id = person.id();
        row.status = person.status();
        row.statusText = person.statusText();
        row.origin = person.originFloor();
        row.destination = person.destinationFloor();
        row.waitSeconds = person.waitSeconds();
        row.elevatorId = person.assignedElevatorId();
        shot.people.push_back(std::move(row));
    }
    return shot;
}

SessionStats Simulation::collectStats() const
{
    SessionStats stats;
    stats.sumWait = sumWait_;
    stats.maxWait = maxWait_;
    stats.waitSamples = waitSamples_;
    stats.avgWait = waitSamples_ > 0 ? sumWait_ / static_cast<double>(waitSamples_) : 0.0;
    stats.fireCount = fireCount_;
    stats.fireDuration = fireDuration_;
    stats.perElevator.reserve(elevators_.size());
    for (const auto& elevator : elevators_) {
        const ElevatorStats row = elevator.stats();
        stats.perElevator.push_back(row);
        stats.totalTrips += row.trips;
        stats.totalTransported += row.transported;
    }
    return stats;
}

void Simulation::rebuildElevators()
{
    elevators_.clear();
    elevators_.reserve(config_.elevators.size());
    for (const auto& spec : config_.elevators) {
        elevators_.emplace_back(spec, config_.floorHeight, 1);
    }
    panels_.clear();
    panels_.reserve(static_cast<std::size_t>(config_.floorCount));
    for (int floor = 1; floor <= config_.floorCount; ++floor) {
        panels_.emplace_back(floor);
    }
}

void Simulation::resetSessionCounters()
{
    sumWait_ = 0.0;
    maxWait_ = 0.0;
    waitSamples_ = 0;
    fireCount_ = 0;
    fireDuration_ = 0.0;
}

void Simulation::applyScenario(double now)
{
    for (const auto& event : scenario_.dueEvents(now)) {
        switch (event.type) {
        case ScenarioEventType::SpawnPeople:
            for (int i = 0; i < event.count && i < kMaxLivingPeople; ++i) {
                if (spawnPerson(event.fromFloor, event.toFloor) < 0) {
                    break;
                }
            }
            break;
        case ScenarioEventType::FireOn:
            setFireAlarm(true);
            break;
        case ScenarioEventType::FireOff:
            setFireAlarm(false);
            break;
        }
    }
}

void Simulation::tickPeople(double dt)
{
    for (auto& person : people_) {
        person.tick(dt);
    }
}

void Simulation::removeExpiredPeople()
{
    for (const auto& person : people_) {
        if (person.shouldRemove()) {
            if (Elevator* elevator = elevatorById(person.assignedElevatorId())) {
                elevator->removePassenger(person.id());
            }
            removeCallsFor(person.id());
        }
    }
    people_.erase(std::remove_if(people_.begin(), people_.end(),
                                 [](const Person& person) { return person.shouldRemove(); }),
                  people_.end());
}

void Simulation::assignCalls()
{
    if (!strategy_) {
        return;
    }
    for (auto& call : pendingCalls_) {
        Person* person = personById(call.personId);
        if (!person || person->status() != PersonStatus::Waiting) {
            continue;
        }
        if (call.assignedElevatorId < 0) {
            call.assignedElevatorId = strategy_->assign(call, elevators_, config_.floorHeight);
            if (call.assignedElevatorId >= 0) {
                person->assignElevator(call.assignedElevatorId);
            }
        }
        if (Elevator* elevator = elevatorById(call.assignedElevatorId)) {
            elevator->addStop(call.originFloor);
        }
    }
}

void Simulation::handleStopsAndPassengers()
{
    for (auto& elevator : elevators_) {
        if (elevator.state() == ElevatorState::FireRecall) {
            continue;
        }
        const int floor = elevator.currentFloor();
        const bool waitersHere = std::any_of(people_.begin(), people_.end(), [&](const Person& person) {
            return person.status() == PersonStatus::Waiting && person.assignedElevatorId() == elevator.id()
                && person.originFloor() == floor;
        });
        if (waitersHere && elevator.state() == ElevatorState::Idle && elevator.atFloor(floor)) {
            elevator.offerDoors();
        }
        if (!elevator.doorsOpen()) {
            continue;
        }
        const auto passengerIds = elevator.passengerIds();
        for (int id : passengerIds) {
            Person* person = personById(id);
            if (person && person->destinationFloor() == floor) {
                elevator.alight(id);
                person->arrive();
            }
        }

        bool boarded = false;
        for (auto& person : people_) {
            if (person.status() != PersonStatus::Waiting || person.assignedElevatorId() != elevator.id()) {
                continue;
            }
            if (person.originFloor() != floor) {
                continue;
            }
            if (!elevator.hasFreeSpace()) {
                continue;
            }
            elevator.board(person.id());
            recordWait(person.waitSeconds());
            person.board(elevator.id());
            elevator.addStop(person.destinationFloor());
            removeCallsFor(person.id());
            boarded = true;
        }
        if (boarded) {
            elevator.pressGo();
        }
    }
}

void Simulation::evacuateOccupants()
{
    for (auto& person : people_) {
        if (person.status() != PersonStatus::Evacuating && person.status() != PersonStatus::Delivered) {
            person.evacuate();
        }
    }
    for (auto& elevator : elevators_) {
        elevator.clearPassengers();
    }
}

Person* Simulation::personById(int id)
{
    return const_cast<Person*>(std::as_const(*this).personById(id));
}

const Person* Simulation::personById(int id) const
{
    auto it = std::find_if(people_.begin(), people_.end(), [id](const Person& person) { return person.id() == id; });
    return it == people_.end() ? nullptr : &(*it);
}

Elevator* Simulation::elevatorById(int id)
{
    return const_cast<Elevator*>(std::as_const(*this).elevatorById(id));
}

const Elevator* Simulation::elevatorById(int id) const
{
    auto it = std::find_if(elevators_.begin(), elevators_.end(),
                           [id](const Elevator& elevator) { return elevator.id() == id; });
    return it == elevators_.end() ? nullptr : &(*it);
}

const FloorPanel* Simulation::panelByFloor(int floor) const
{
    auto it = std::find_if(panels_.begin(), panels_.end(),
                           [floor](const FloorPanel& panel) { return panel.floor() == floor; });
    return it == panels_.end() ? nullptr : &(*it);
}

void Simulation::recordWait(double seconds)
{
    sumWait_ += seconds;
    maxWait_ = std::max(maxWait_, seconds);
    ++waitSamples_;
}

void Simulation::removeCallsFor(int personId)
{
    pendingCalls_.erase(std::remove_if(pendingCalls_.begin(), pendingCalls_.end(),
                                       [personId](const HallCall& call) { return call.personId == personId; }),
                        pendingCalls_.end());
}

}  // namespace sim
