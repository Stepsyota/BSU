#pragma once

#include "BuildingConfig.h"
#include "Elevator.h"
#include "FloorPanel.h"
#include "IDispatchStrategy.h"
#include "Person.h"
#include "Scenario.h"
#include "Statistics.h"
#include "Types.h"

#include <memory>
#include <string>
#include <vector>

namespace sim {

struct ElevatorRow {
    int id = 0;
    int floor = 1;
    double positionM = 0.0;
    double velocity = 0.0;
    ElevatorState state = ElevatorState::Idle;
    bool doorsOpen = false;
    int passengers = 0;
    int capacity = 0;
    int nextStop = 0;
    std::string stops;
    int trips = 0;
    double idlePercent = 0.0;
};

struct PersonRow {
    int id = 0;
    PersonStatus status = PersonStatus::Created;
    std::string statusText;
    int origin = 1;
    int destination = 0;
    double waitSeconds = 0.0;
    int elevatorId = -1;
};

struct SessionSnapshot {
    SessionState state = SessionState::Stopped;
    double timeSeconds = 0.0;
    int transported = 0;
    bool fireAlarm = false;
    bool canStop = true;
    std::string strategyName;
    double speedFactor = 1.0;
    int floorCount = 10;
    double floorHeight = 3.0;
    std::vector<ElevatorRow> elevators;
    std::vector<PersonRow> people;
};

class Simulation {
public:
    Simulation();

    bool configure(BuildingConfig config);
    const BuildingConfig& config() const { return config_; }

    void setStrategy(std::unique_ptr<IDispatchStrategy> strategy);
    const IDispatchStrategy* strategy() const { return strategy_.get(); }

    bool start();
    void pause();
    void resume();
    bool tryStop();
    bool canStop() const;

    void setSpeedFactor(double factor);
    double speedFactor() const { return speedFactor_; }

    void setFireAlarm(bool on);
    bool fireAlarm() const { return fireAlarm_; }

    void setScenario(Scenario scenario);
    const Scenario& scenario() const { return scenario_; }

    int spawnPerson(int originFloor, int destinationFloor = 0);
    bool submitDestination(int personId, int destFloor);
    int personCount() const { return static_cast<int>(people_.size()); }

    void tick(double realDt);

    SessionSnapshot snapshot() const;
    SessionStats collectStats() const;
    SessionState state() const { return state_; }

private:
    void rebuildElevators();
    void resetSessionCounters();
    void applyScenario(double now);
    void tickPeople(double dt);
    void removeExpiredPeople();
    void assignCalls();
    void handleStopsAndPassengers();
    void evacuateOccupants();
    Person* personById(int id);
    const Person* personById(int id) const;
    Elevator* elevatorById(int id);
    const Elevator* elevatorById(int id) const;
    const FloorPanel* panelByFloor(int floor) const;
    void recordWait(double seconds);
    void removeCallsFor(int personId);

    BuildingConfig config_ = BuildingConfig::defaults();
    std::vector<Elevator> elevators_;
    std::vector<Person> people_;
    std::vector<FloorPanel> panels_;
    std::vector<HallCall> pendingCalls_;
    std::unique_ptr<IDispatchStrategy> strategy_;
    Scenario scenario_;
    SessionState state_ = SessionState::Stopped;
    bool fireAlarm_ = false;
    double timeSeconds_ = 0.0;
    double speedFactor_ = 1.0;
    int nextPersonId_ = 1;
    double sumWait_ = 0.0;
    double maxWait_ = 0.0;
    int waitSamples_ = 0;
    int fireCount_ = 0;
    double fireDuration_ = 0.0;
};

}  // namespace sim
