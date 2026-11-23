#include "MainWindow.h"

#include "Constants.h"
#include "MinIdleStrategy.h"
#include "MinWaitStrategy.h"
#include "JsonStore.h"
#include "BuildingView.h"
#include "ConfigDialog.h"
#include "StatsDialog.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QShortcut>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <memory>
#include <utility>

namespace {

QString elevatorStateText(sim::ElevatorState state)
{
    switch (state) {
    case sim::ElevatorState::Idle:
        return QStringLiteral("стоит");
    case sim::ElevatorState::Moving:
        return QStringLiteral("едет");
    case sim::ElevatorState::DoorsOpen:
        return QStringLiteral("двери открыты");
    case sim::ElevatorState::FireRecall:
        return QStringLiteral("пожарная эвакуация");
    }
    return {};
}

void setTableRow(QTableWidget* table, int row, const QStringList& values)
{
    if (row >= table->rowCount()) {
        table->setRowCount(row + 1);
    }
    for (int column = 0; column < values.size(); ++column) {
        auto* item = table->item(row, column);
        if (!item) {
            item = new QTableWidgetItem(values[column]);
            table->setItem(row, column, item);
        } else {
            item->setText(values[column]);
        }
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    }
}

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Лифты — лабораторная 8"));
    resize(1280, 780);
    buildUi();
    connectSignals();
    auto* tabShortcut = new QShortcut(Qt::Key_Tab, this);
    tabShortcut->setContext(Qt::WindowShortcut);
    connect(tabShortcut, &QShortcut::activated, this, [this]() {
        if (spawnStep_ == SpawnStep::Idle) {
            enterSpawnMode();
        } else {
            exitSpawnMode();
        }
    });
    refresh();
}

void MainWindow::buildUi()
{
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("Файл"));
    fileMenu->addAction(QStringLiteral("Импорт конфигурации…"), this, &MainWindow::importConfig);
    fileMenu->addAction(QStringLiteral("Экспорт конфигурации…"), this, &MainWindow::exportConfig);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("Импорт сценария…"), this, &MainWindow::importScenario);
    fileMenu->addAction(QStringLiteral("Экспорт сценария…"), this, &MainWindow::exportScenario);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("Статистика сеанса…"), this, &MainWindow::showStats);
    fileMenu->addAction(QStringLiteral("Экспорт отчёта…"), this, &MainWindow::exportReport);

    auto* settingsMenu = menuBar()->addMenu(QStringLiteral("Настройки"));
    settingsMenu->addAction(QStringLiteral("Здание и лифты…"), this, &MainWindow::openConfig);

    auto* controls = new QGroupBox(QStringLiteral("Управление сеансом"), this);
    auto* controlsLayout = new QVBoxLayout(controls);

    auto* buttons = new QHBoxLayout();
    startButton_ = new QPushButton(QStringLiteral("Старт"), controls);
    pauseButton_ = new QPushButton(QStringLiteral("Пауза"), controls);
    stopButton_ = new QPushButton(QStringLiteral("Стоп"), controls);
    startButton_->setAutoDefault(false);
    pauseButton_->setAutoDefault(false);
    stopButton_->setAutoDefault(false);
    buttons->addWidget(startButton_);
    buttons->addWidget(pauseButton_);
    buttons->addWidget(stopButton_);
    controlsLayout->addLayout(buttons);

    strategyCombo_ = new QComboBox(controls);
    strategyCombo_->addItem(QStringLiteral("Минимум времени ожидания"), QStringLiteral("min-wait"));
    strategyCombo_->addItem(QStringLiteral("Минимум холостых ходов"), QStringLiteral("min-idle"));
    controlsLayout->addWidget(new QLabel(QStringLiteral("Стратегия:"), controls));
    controlsLayout->addWidget(strategyCombo_);

    speedSlider_ = new QSlider(Qt::Horizontal, controls);
    speedSlider_->setRange(static_cast<int>(sim::kMinSpeedFactor * 100), static_cast<int>(sim::kMaxSpeedFactor * 100));
    speedSlider_->setValue(100);
    speedLabel_ = new QLabel(QStringLiteral("Скорость: 1.00×"), controls);
    controlsLayout->addWidget(speedLabel_);
    controlsLayout->addWidget(speedSlider_);

    fireCheck_ = new QCheckBox(QStringLiteral("Пожарная тревога"), controls);
    controlsLayout->addWidget(fireCheck_);

    auto* spawnBox = new QGroupBox(QStringLiteral("Создать человека"), controls);
    auto* spawnForm = new QFormLayout(spawnBox);
    originSpin_ = new QSpinBox(spawnBox);
    destSpin_ = new QSpinBox(spawnBox);
    originSpin_->setRange(1, sim::kMaxFloors);
    destSpin_->setRange(1, sim::kMaxFloors);
    originSpin_->setValue(1);
    destSpin_->setValue(2);
    spawnButton_ = new QPushButton(QStringLiteral("Создать"), spawnBox);
    spawnButton_->setAutoDefault(false);
    spawnButton_->setDefault(false);
    spawnHint_ = new QLabel(QStringLiteral("Tab — добавить с клавиатуры"), spawnBox);
    spawnHint_->setWordWrap(true);
    spawnForm->addRow(QStringLiteral("Откуда"), originSpin_);
    spawnForm->addRow(QStringLiteral("Куда"), destSpin_);
    spawnForm->addRow(spawnButton_);
    spawnForm->addRow(spawnHint_);
    controlsLayout->addWidget(spawnBox);
    controlsLayout->addStretch();

    elevatorTable_ = new QTableWidget(this);
    elevatorTable_->setColumnCount(9);
    elevatorTable_->setHorizontalHeaderLabels({QStringLiteral("Id"), QStringLiteral("Этаж"), QStringLiteral("v, м/с"),
                                               QStringLiteral("Состояние"), QStringLiteral("Двери"),
                                               QStringLiteral("Люди"), QStringLiteral("Остановки"),
                                               QStringLiteral("Поездки"), QStringLiteral("Холостые %")});
    elevatorTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    elevatorTable_->verticalHeader()->setVisible(false);

    peopleTable_ = new QTableWidget(this);
    peopleTable_->setColumnCount(6);
    peopleTable_->setHorizontalHeaderLabels({QStringLiteral("Id"), QStringLiteral("Статус"), QStringLiteral("Откуда"),
                                             QStringLiteral("Куда"), QStringLiteral("Ожидание, с"),
                                             QStringLiteral("Лифт")});
    peopleTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    peopleTable_->verticalHeader()->setVisible(false);

    buildingView_ = new BuildingView(this);

    auto* tables = new QSplitter(Qt::Vertical, this);
    tables->addWidget(elevatorTable_);
    tables->addWidget(peopleTable_);
    tables->setStretchFactor(0, 1);
    tables->setStretchFactor(1, 1);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(controls);
    splitter->addWidget(buildingView_);
    splitter->addWidget(tables);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 1);
    splitter->setSizes({240, 520, 420});
    setCentralWidget(splitter);

    statusBar()->showMessage(QStringLiteral("Время: 0 с | Перевезено: 0"));

    timer_ = new QTimer(this);
    timer_->setInterval(static_cast<int>(sim::kTickSeconds * 1000));
    timer_->start();
}

void MainWindow::connectSignals()
{
    connect(timer_, &QTimer::timeout, this, [this]() {
        simulation_.tick(sim::kTickSeconds);
        refresh();
    });
    connect(startButton_, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(pauseButton_, &QPushButton::clicked, this, &MainWindow::onPause);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(spawnButton_, &QPushButton::clicked, this, &MainWindow::onSpawn);
    connect(fireCheck_, &QCheckBox::toggled, this, &MainWindow::onFireToggled);
    connect(speedSlider_, &QSlider::valueChanged, this, &MainWindow::onSpeedChanged);
}

void MainWindow::refresh()
{
    const sim::SessionSnapshot shot = simulation_.snapshot();
    elevatorTable_->setRowCount(static_cast<int>(shot.elevators.size()));
    for (int row = 0; row < static_cast<int>(shot.elevators.size()); ++row) {
        const auto& e = shot.elevators[static_cast<std::size_t>(row)];
        setTableRow(elevatorTable_, row,
                    {QString::number(e.id), QString::number(e.floor), QString::number(e.velocity, 'f', 2),
                     elevatorStateText(e.state), e.doorsOpen ? QStringLiteral("открыты") : QStringLiteral("закрыты"),
                     QStringLiteral("%1/%2").arg(e.passengers).arg(e.capacity),
                     e.stops.empty() ? QStringLiteral("—") : QString::fromStdString(e.stops), QString::number(e.trips),
                     QString::number(e.idlePercent, 'f', 1)});
    }

    peopleTable_->setRowCount(static_cast<int>(shot.people.size()));
    for (int row = 0; row < static_cast<int>(shot.people.size()); ++row) {
        const auto& p = shot.people[static_cast<std::size_t>(row)];
        setTableRow(peopleTable_, row,
                    {QString::number(p.id), QString::fromStdString(p.statusText), QString::number(p.origin),
                     p.destination > 0 ? QString::number(p.destination) : QStringLiteral("—"),
                     QString::number(p.waitSeconds, 'f', 1),
                     p.elevatorId > 0 ? QString::number(p.elevatorId) : QStringLiteral("—")});
    }

    statusBar()->showMessage(QStringLiteral("Время: %1 с | Перевезено: %2")
                                 .arg(QString::number(shot.timeSeconds, 'f', 1))
                                 .arg(shot.transported));
    buildingView_->setSnapshot(shot);
    updateSpawnUi();
    syncControls(shot);
}

void MainWindow::syncControls(const sim::SessionSnapshot& shot)
{
    const bool stopped = shot.state == sim::SessionState::Stopped;
    const bool running = shot.state == sim::SessionState::Running;
    startButton_->setEnabled(stopped);
    pauseButton_->setEnabled(running || shot.state == sim::SessionState::Paused);
    pauseButton_->setText(shot.state == sim::SessionState::Paused ? QStringLiteral("Продолжить")
                                                                  : QStringLiteral("Пауза"));
    stopButton_->setEnabled(!stopped);
    strategyCombo_->setEnabled(stopped);
    fireCheck_->blockSignals(true);
    fireCheck_->setChecked(shot.fireAlarm);
    fireCheck_->setEnabled(!stopped);
    fireCheck_->blockSignals(false);

    originSpin_->setRange(1, shot.floorCount);
    destSpin_->setRange(1, shot.floorCount);
}

void MainWindow::onStart()
{
    applyStrategyFromCombo();
    if (!simulation_.start()) {
        QMessageBox::warning(this, QStringLiteral("Старт"), QStringLiteral("Не удалось запустить симуляцию."));
        return;
    }
    refresh();
}

void MainWindow::onPause()
{
    if (simulation_.state() == sim::SessionState::Running) {
        simulation_.pause();
    } else if (simulation_.state() == sim::SessionState::Paused) {
        simulation_.resume();
    }
    refresh();
}

void MainWindow::onStop()
{
    if (!simulation_.tryStop()) {
        QMessageBox::information(this, QStringLiteral("Остановка"),
                                 QStringLiteral("Остановить систему можно только если все лифты пустые и остановлены."));
        return;
    }
    refresh();
    StatsDialog dialog(simulation_.collectStats(), this);
    dialog.exec();
}

void MainWindow::onSpawn()
{
    const int origin = originSpin_->value();
    const int dest = destSpin_->value();
    if (simulation_.spawnPerson(origin, dest) < 0) {
        statusBar()->showMessage(
            QStringLiteral("Не удалось создать пассажира (лимит %1 или этажи совпадают)").arg(sim::kMaxLivingPeople),
            3000);
        return;
    }
    refresh();
}

void MainWindow::onFireToggled(bool on)
{
    simulation_.setFireAlarm(on);
    refresh();
}

void MainWindow::onSpeedChanged(int value)
{
    const double factor = value / 100.0;
    simulation_.setSpeedFactor(factor);
    speedLabel_->setText(QStringLiteral("Скорость: %1×").arg(factor, 0, 'f', 2));
}

void MainWindow::openConfig()
{
    if (simulation_.state() != sim::SessionState::Stopped) {
        QMessageBox::information(this, QStringLiteral("Конфигурация"),
                                 QStringLiteral("Менять здание можно только при остановленной симуляции."));
        return;
    }
    ConfigDialog dialog(simulation_.config(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const sim::BuildingConfig config = dialog.config();
    if (!simulation_.configure(config)) {
        QMessageBox::warning(this, QStringLiteral("Конфигурация"),
                             QStringLiteral("Параметры не проходят ограничения ТЗ (этажи 1–20, лифты 1–5)."));
        return;
    }
    refresh();
}

void MainWindow::importConfig()
{
    if (simulation_.state() != sim::SessionState::Stopped) {
        QMessageBox::information(this, QStringLiteral("Импорт"),
                                 QStringLiteral("Импорт конфигурации доступен только при остановке."));
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Импорт конфигурации"), {},
                                                      QStringLiteral("JSON (*.json);;Все файлы (*)"));
    if (path.isEmpty()) {
        return;
    }
    sim::BuildingConfig config;
    QString error;
    if (!io::JsonStore::loadConfig(path, config, &error) || !simulation_.configure(config)) {
        QMessageBox::warning(this, QStringLiteral("Импорт"), error.isEmpty() ? QStringLiteral("Некорректный файл") : error);
        return;
    }
    refresh();
}

void MainWindow::exportConfig()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Экспорт конфигурации"),
                                                      QStringLiteral("building.json"),
                                                      QStringLiteral("JSON (*.json);;Все файлы (*)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!io::JsonStore::saveConfig(path, simulation_.config(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Экспорт"), error);
    }
}

void MainWindow::importScenario()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Импорт сценария"), {},
                                                      QStringLiteral("JSON (*.json);;Все файлы (*)"));
    if (path.isEmpty()) {
        return;
    }
    sim::Scenario scenario;
    QString error;
    if (!io::JsonStore::loadScenario(path, scenario, &error)) {
        QMessageBox::warning(this, QStringLiteral("Импорт"), error);
        return;
    }
    simulation_.setScenario(std::move(scenario));
}

void MainWindow::exportScenario()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Экспорт сценария"),
                                                      QStringLiteral("scenario.json"),
                                                      QStringLiteral("JSON (*.json);;Все файлы (*)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!io::JsonStore::saveScenario(path, simulation_.scenario(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Экспорт"), error);
    }
}

void MainWindow::showStats()
{
    StatsDialog dialog(simulation_.collectStats(), this);
    dialog.exec();
}

void MainWindow::exportReport()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Экспорт отчёта"), QStringLiteral("report.txt"),
                                                      QStringLiteral("Текст (*.txt);;Все файлы (*)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!io::JsonStore::saveReport(path, simulation_.collectStats(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Экспорт"), error);
    }
}

void MainWindow::applyStrategyFromCombo()
{
    const QString key = strategyCombo_->currentData().toString();
    if (key == QLatin1String("min-idle")) {
        simulation_.setStrategy(std::make_unique<sim::MinIdleStrategy>());
    } else {
        simulation_.setStrategy(std::make_unique<sim::MinWaitStrategy>());
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (handleSpawnKeys(event)) {
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::ActivationChange && !isActiveWindow() && spawnStep_ != SpawnStep::Idle) {
        exitSpawnMode();
    }
}

bool MainWindow::handleSpawnKeys(QKeyEvent* event)
{
    const int key = event->key();
    if (key == Qt::Key_Tab) {
        if (event->isAutoRepeat()) {
            return true;
        }
        if (spawnStep_ == SpawnStep::Idle) {
            enterSpawnMode();
        } else {
            exitSpawnMode();
        }
        return true;
    }
    if (key == Qt::Key_Escape && spawnStep_ != SpawnStep::Idle) {
        exitSpawnMode();
        return true;
    }
    if (spawnStep_ == SpawnStep::Idle) {
        return false;
    }

    switch (key) {
    case Qt::Key_W:
    case Qt::Key_Up:
        bumpSpawnFloor(1);
        return true;
    case Qt::Key_S:
    case Qt::Key_Down:
        bumpSpawnFloor(-1);
        return true;
    case Qt::Key_A:
    case Qt::Key_Left:
        spawnStep_ = SpawnStep::Origin;
        updateSpawnUi();
        return true;
    case Qt::Key_D:
    case Qt::Key_Right:
        spawnStep_ = SpawnStep::Destination;
        updateSpawnUi();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (event->isAutoRepeat()) {
            return true;
        }
        if (spawnStep_ == SpawnStep::Origin) {
            spawnStep_ = SpawnStep::Destination;
            if (destSpin_->value() == originSpin_->value()) {
                destSpin_->setValue(std::min(originSpin_->value() + 1, floorCount()));
                if (destSpin_->value() == originSpin_->value()) {
                    destSpin_->setValue(std::max(1, originSpin_->value() - 1));
                }
            }
            updateSpawnUi();
            return true;
        }
        onSpawn();
        spawnStep_ = SpawnStep::Origin;
        updateSpawnUi();
        return true;
    default:
        return false;
    }
}

void MainWindow::enterSpawnMode()
{
    spawnStep_ = SpawnStep::Origin;
    grabKeyboard();
    updateSpawnUi();
}

void MainWindow::exitSpawnMode()
{
    spawnStep_ = SpawnStep::Idle;
    releaseKeyboard();
    updateSpawnUi();
}

void MainWindow::bumpSpawnFloor(int delta)
{
    QSpinBox* spin = spawnStep_ == SpawnStep::Destination ? destSpin_ : originSpin_;
    spin->setValue(std::clamp(spin->value() + delta, 1, floorCount()));
    updateSpawnUi();
}

void MainWindow::updateSpawnUi()
{
    const bool active = spawnStep_ != SpawnStep::Idle;
    if (spawnHint_) {
        if (!active) {
            spawnHint_->setText(QStringLiteral("Tab — режим добавления, Esc — выход"));
        } else if (spawnStep_ == SpawnStep::Origin) {
            spawnHint_->setText(QStringLiteral("Откуда: W/S этаж, Enter — куда"));
        } else {
            spawnHint_->setText(QStringLiteral("Куда: W/S этаж, Enter — создать"));
        }
    }
    if (buildingView_) {
        buildingView_->setSpawnPreview(active, originSpin_->value(), destSpin_->value(),
                                       spawnStep_ != SpawnStep::Destination);
    }
}

int MainWindow::floorCount() const
{
    return std::max(1, simulation_.config().floorCount);
}
