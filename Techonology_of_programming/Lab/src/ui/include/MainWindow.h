#pragma once

#include "Simulation.h"

#include <QMainWindow>

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

private:
    void buildUi();
    void connectSignals();
    void refresh();
    void syncControls();
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
    QTableWidget* elevatorTable_ = nullptr;
    QTableWidget* peopleTable_ = nullptr;
};
