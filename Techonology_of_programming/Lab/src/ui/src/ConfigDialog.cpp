#include "ConfigDialog.h"

#include "Constants.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include <utility>

ConfigDialog::ConfigDialog(sim::BuildingConfig config, QWidget* parent)
    : QDialog(parent)
    , config_(std::move(config))
{
    setWindowTitle(QStringLiteral("Конфигурация здания и лифтов"));
    resize(640, 420);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    floorCount_ = new QSpinBox(this);
    floorCount_->setRange(sim::kMinFloors, sim::kMaxFloors);
    floorCount_->setValue(config_.floorCount);
    form->addRow(QStringLiteral("Этажей"), floorCount_);

    elevatorCount_ = new QSpinBox(this);
    elevatorCount_->setRange(sim::kMinElevators, sim::kMaxElevators);
    elevatorCount_->setValue(static_cast<int>(config_.elevators.size()));
    form->addRow(QStringLiteral("Лифтов"), elevatorCount_);

    floorHeight_ = new QDoubleSpinBox(this);
    floorHeight_->setRange(1.0, 10.0);
    floorHeight_->setDecimals(2);
    floorHeight_->setSuffix(QStringLiteral(" м"));
    floorHeight_->setValue(config_.floorHeight);
    form->addRow(QStringLiteral("Высота этажа"), floorHeight_);

    layout->addLayout(form);

    elevatorTable_ = new QTableWidget(this);
    elevatorTable_->setColumnCount(4);
    elevatorTable_->setHorizontalHeaderLabels(
        {QStringLiteral("Id"), QStringLiteral("Вместимость"), QStringLiteral("v макс, м/с"),
         QStringLiteral("a макс, м/с²")});
    elevatorTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(new QLabel(QStringLiteral("Параметры лифтов:"), this));
    layout->addWidget(elevatorTable_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(elevatorCount_, &QSpinBox::valueChanged, this, [this](int) { rebuildElevatorTable(); });
    rebuildElevatorTable();
}

sim::BuildingConfig ConfigDialog::config() const
{
    sim::BuildingConfig result;
    result.floorCount = floorCount_->value();
    result.floorHeight = floorHeight_->value();
    const int count = elevatorCount_->value();
    result.elevators.resize(static_cast<std::size_t>(count));
    for (int row = 0; row < count; ++row) {
        auto cell = [&](int column) -> QTableWidgetItem* { return elevatorTable_->item(row, column); };
        sim::ElevatorSpec spec;
        spec.id = cell(0) ? cell(0)->text().toInt() : row + 1;
        spec.capacity = cell(1) ? cell(1)->text().toInt() : sim::kDefaultCapacity;
        spec.maxSpeed = cell(2) ? cell(2)->text().toDouble() : sim::kDefaultMaxSpeed;
        spec.maxAcceleration = cell(3) ? cell(3)->text().toDouble() : sim::kDefaultMaxAccel;
        if (spec.id <= 0) {
            spec.id = row + 1;
        }
        result.elevators[static_cast<std::size_t>(row)] = spec;
    }
    return result;
}

void ConfigDialog::rebuildElevatorTable()
{
    const int count = elevatorCount_->value();
    elevatorTable_->setRowCount(count);
    for (int row = 0; row < count; ++row) {
        sim::ElevatorSpec spec;
        if (row < static_cast<int>(config_.elevators.size())) {
            spec = config_.elevators[static_cast<std::size_t>(row)];
        } else {
            spec.id = row + 1;
            spec.capacity = sim::kDefaultCapacity;
            spec.maxSpeed = sim::kDefaultMaxSpeed;
            spec.maxAcceleration = sim::kDefaultMaxAccel;
        }
        auto setCell = [&](int column, const QString& text) {
            auto* item = elevatorTable_->item(row, column);
            if (!item) {
                item = new QTableWidgetItem(text);
                elevatorTable_->setItem(row, column, item);
            } else if (item->text().isEmpty()) {
                item->setText(text);
            }
        };
        if (!elevatorTable_->item(row, 0)) {
            setCell(0, QString::number(spec.id));
            setCell(1, QString::number(spec.capacity));
            setCell(2, QString::number(spec.maxSpeed, 'f', 2));
            setCell(3, QString::number(spec.maxAcceleration, 'f', 2));
        }
    }
}
