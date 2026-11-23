#pragma once

#include "BuildingConfig.h"

#include <QDialog>

class QSpinBox;
class QDoubleSpinBox;
class QTableWidget;

class ConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigDialog(sim::BuildingConfig config, QWidget* parent = nullptr);

    sim::BuildingConfig config() const;

private:
    void rebuildElevatorTable();

    sim::BuildingConfig config_;
    QSpinBox* floorCount_ = nullptr;
    QSpinBox* elevatorCount_ = nullptr;
    QDoubleSpinBox* floorHeight_ = nullptr;
    QTableWidget* elevatorTable_ = nullptr;
};
