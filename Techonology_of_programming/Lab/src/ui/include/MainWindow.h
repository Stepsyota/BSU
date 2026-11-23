#pragma once

#include "Simulation.h"

#include <QMainWindow>

class QKeyEvent;

class BuildingView;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QTableWidget;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    enum class SpawnStep { Idle, Origin, Destination };

    void buildUi();
    void connectSignals();
    void refresh();
    void syncControls(const sim::SessionSnapshot& shot);
    void onStart();
    void onPause();
    void onStop();
    void onSpawn();
    void onFireToggled(bool on);
    void onSpeedChanged(int value);
    void openConfig();
    void importConfig();
    void exportConfig();
    void importScenario();
    void exportScenario();
    void showStats();
    void exportReport();
    void applyStrategyFromCombo();
    bool handleSpawnKeys(QKeyEvent* event);
    void enterSpawnMode();
    void exitSpawnMode();
    void bumpSpawnFloor(int delta);
    void updateSpawnUi();
    int floorCount() const;

    sim::Simulation simulation_;
    QTimer* timer_ = nullptr;

    QPushButton* startButton_ = nullptr;
    QPushButton* pauseButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QComboBox* strategyCombo_ = nullptr;
    QSlider* speedSlider_ = nullptr;
    QLabel* speedLabel_ = nullptr;
    QCheckBox* fireCheck_ = nullptr;
    QSpinBox* originSpin_ = nullptr;
    QSpinBox* destSpin_ = nullptr;
    QPushButton* spawnButton_ = nullptr;
    QLabel* spawnHint_ = nullptr;
    BuildingView* buildingView_ = nullptr;
    QTableWidget* elevatorTable_ = nullptr;
    QTableWidget* peopleTable_ = nullptr;

    SpawnStep spawnStep_ = SpawnStep::Idle;
};
