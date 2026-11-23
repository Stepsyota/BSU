#include "BuildingView.h"

#include <QPainter>
#include <QPaintEvent>

#include <algorithm>
#include <map>
#include <vector>

namespace {

QColor cabinColor(sim::ElevatorState state)
{
    switch (state) {
    case sim::ElevatorState::Moving:
        return QColor(70, 130, 200);
    case sim::ElevatorState::DoorsOpen:
        return QColor(70, 160, 90);
    case sim::ElevatorState::FireRecall:
        return QColor(190, 70, 60);
    case sim::ElevatorState::Idle:
        break;
    }
    return QColor(150, 155, 165);
}

QColor personColor(sim::PersonStatus status)
{
    switch (status) {
    case sim::PersonStatus::Waiting:
        return QColor(220, 170, 40);
    case sim::PersonStatus::Moving:
        return QColor(60, 110, 200);
    case sim::PersonStatus::Delivered:
        return QColor(50, 150, 80);
    case sim::PersonStatus::Evacuating:
        return QColor(210, 90, 40);
    case sim::PersonStatus::Created:
        break;
    }
    return QColor(120, 120, 130);
}

int displayFloor(const sim::PersonRow& person)
{
    if (person.status == sim::PersonStatus::Delivered && person.destination > 0) {
        return person.destination;
    }
    return person.origin;
}

}  // namespace

BuildingView::BuildingView(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(280, 360);
    setAutoFillBackground(true);
}

void BuildingView::setSnapshot(const sim::SessionSnapshot& snapshot)
{
    snapshot_ = snapshot;
    update();
}

void BuildingView::setSpawnPreview(bool active, int origin, int dest, bool editingOrigin)
{
    spawnActive_ = active;
    spawnOrigin_ = origin;
    spawnDest_ = dest;
    spawnEditingOrigin_ = editingOrigin;
    update();
}

QSize BuildingView::minimumSizeHint() const
{
    return {280, 360};
}

QSize BuildingView::sizeHint() const
{
    return {420, 640};
}

void BuildingView::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF area = QRectF(rect()).adjusted(8, 8, -8, -8);
    const QColor wall = snapshot_.fireAlarm ? QColor(255, 228, 220) : QColor(245, 246, 248);
    painter.fillRect(rect(), snapshot_.fireAlarm ? QColor(255, 236, 230) : QColor(232, 234, 238));
    painter.setPen(QPen(QColor(80, 80, 90), 1.5));
    painter.setBrush(wall);
    painter.drawRoundedRect(area, 8, 8);

    const int floors = std::max(1, snapshot_.floorCount);
    const int shafts = std::max(1, static_cast<int>(snapshot_.elevators.size()));
    const double floorHeightM = snapshot_.floorHeight > 0.0 ? snapshot_.floorHeight : 3.0;

    const double leftPad = 28;
    const double hallWidth = 118;
    const double titleH = spawnActive_ ? 42 : 22;
    const double bottomPad = 16;
    const QRectF inner(area.left() + 10, area.top() + titleH, area.width() - 20, area.height() - titleH - bottomPad);

    painter.setPen(QColor(50, 50, 60));
    QFont title = font();
    title.setBold(true);
    painter.setFont(title);
    painter.drawText(QRectF(area.left(), area.top() + 2, area.width(), 20), Qt::AlignCenter,
                     snapshot_.fireAlarm ? QStringLiteral("Здание — пожарная тревога") : QStringLiteral("Здание"));
    if (spawnActive_) {
        QFont hint = font();
        hint.setBold(false);
        painter.setFont(hint);
        const QString fromMark = spawnEditingOrigin_ ? QStringLiteral("[%1]") : QStringLiteral("%1");
        const QString toMark = spawnEditingOrigin_ ? QStringLiteral("%1") : QStringLiteral("[%1]");
        painter.setPen(QColor(20, 90, 140));
        painter.drawText(QRectF(area.left(), area.top() + 20, area.width(), 20), Qt::AlignCenter,
                         QStringLiteral("Добавление: %1 → %2   W/S этаж, A/D поле, Enter")
                             .arg(fromMark.arg(spawnOrigin_))
                             .arg(toMark.arg(spawnDest_)));
    }

    const double floorPx = inner.height() / floors;
    const double shaftGap = 6;
    const double shaftsLeft = inner.left() + leftPad + hallWidth;
    const double shaftsWidth = std::max(40.0, inner.right() - shaftsLeft - 8);
    constexpr double kCellW = 28;
    constexpr double kCellH = 24;
    constexpr int kCabinCols = 4;
    constexpr int kCabinRows = 2;
    const double headerH = 16;
    const double neededCabinW = kCabinCols * kCellW + 8;
    const double neededCabinH = headerH + kCabinRows * kCellH + 6;
    const double shaftW = std::max(neededCabinW, (shaftsWidth - shaftGap * (shafts - 1)) / std::max(1, shafts));
    const double cabinH = std::max(neededCabinH, std::min(floorPx * 0.95, neededCabinH + 8));
    const double personScale = std::clamp(floorPx / 36.0, 0.55, 1.0);

    auto floorBaseline = [&](int floor) { return inner.bottom() - (floor - 1) * floorPx; };
    auto cabinTop = [&](double positionM) {
        return inner.bottom() - (positionM / floorHeightM) * floorPx - cabinH;
    };

    QFont floorFont = font();
    floorFont.setPointSize(std::max(8, font().pointSize() - 1));
    painter.setFont(floorFont);

    for (int floor = 1; floor <= floors; ++floor) {
        const double y = floorBaseline(floor);
        QColor line = QColor(200, 200, 208);
        if (spawnActive_ && (floor == spawnOrigin_ || floor == spawnDest_)) {
            line = QColor(80, 140, 200);
        }
        const bool activeFloor = spawnActive_ && floor == (spawnEditingOrigin_ ? spawnOrigin_ : spawnDest_);
        painter.setPen(QPen(line, activeFloor ? 2 : 1, Qt::DashLine));
        painter.drawLine(QPointF(inner.left() + leftPad, y), QPointF(inner.right(), y));
        painter.setPen(QColor(90, 90, 100));
        painter.drawText(QRectF(inner.left(), y - floorPx, leftPad - 2, floorPx), Qt::AlignCenter, QString::number(floor));
    }

    std::map<int, QRectF> cabinRects;
    for (int i = 0; i < shafts; ++i) {
        const auto& elevator = snapshot_.elevators[static_cast<std::size_t>(i)];
        const double x = shaftsLeft + i * (shaftW + shaftGap);
        const QRectF shaft(x, inner.top(), shaftW, inner.height());
        painter.setPen(QPen(QColor(170, 172, 180), 1));
        painter.setBrush(QColor(210, 214, 220));
        painter.drawRect(shaft);

        const double cabinY = std::clamp(cabinTop(elevator.positionM), inner.top(), inner.bottom() - cabinH);
        const QRectF cabin(x + 3, cabinY, shaftW - 6, cabinH);
        cabinRects[elevator.id] = cabin;
        painter.setPen(QPen(QColor(40, 40, 50), 1.2));
        painter.setBrush(cabinColor(elevator.state));
        painter.drawRoundedRect(cabin, 4, 4);

        painter.setPen(Qt::white);
        QFont cabinFont = font();
        cabinFont.setBold(true);
        cabinFont.setPointSize(std::max(8, font().pointSize() - 1));
        painter.setFont(cabinFont);
        QString header = QStringLiteral("Л%1  %2/%3").arg(elevator.id).arg(elevator.passengers).arg(elevator.capacity);
        if (elevator.nextStop > 0) {
            header += QStringLiteral("  →%1").arg(elevator.nextStop);
        }
        painter.drawText(QRectF(cabin.left(), cabin.top() + 1, cabin.width(), headerH - 2), Qt::AlignCenter, header);
    }

    std::map<int, std::vector<const sim::PersonRow*>> hallByFloor;
    std::map<int, std::vector<const sim::PersonRow*>> cabinByElevator;
    for (const auto& person : snapshot_.people) {
        if (person.status == sim::PersonStatus::Moving) {
            cabinByElevator[person.elevatorId].push_back(&person);
        } else {
            hallByFloor[displayFloor(person)].push_back(&person);
        }
    }

    const double figureW = 40.0 * personScale;
    for (int floor = 1; floor <= floors; ++floor) {
        const auto& people = hallByFloor[floor];
        if (people.empty()) {
            continue;
        }
        const QRectF band(inner.left() + leftPad + 2, floorBaseline(floor) - floorPx + 1, hallWidth - 6, floorPx - 2);
        painter.save();
        painter.setClipRect(band);
        const int maxVisible = std::max(1, static_cast<int>((band.width() - 22) / figureW));
        const int shown = std::min(static_cast<int>(people.size()), maxVisible);
        const int hidden = static_cast<int>(people.size()) - shown;
        for (int i = 0; i < shown; ++i) {
            const double x = band.left() + 8 + i * figureW;
            drawPerson(painter, QPointF(x, band.bottom() - 2), *people[static_cast<std::size_t>(i)], personScale);
        }
        if (hidden > 0) {
            drawOverflow(painter, QPointF(band.right() - 12, band.center().y()), hidden);
        }
        painter.restore();
    }

    for (const auto& [elevatorId, people] : cabinByElevator) {
        const auto it = cabinRects.find(elevatorId);
        if (it == cabinRects.end()) {
            continue;
        }
        const QRectF cabin = it->second;
        const QRectF grid(cabin.left() + 4, cabin.top() + headerH, cabin.width() - 8, cabin.height() - headerH - 2);
        painter.save();
        painter.setClipRect(cabin);
        const int count = std::min(static_cast<int>(people.size()), kCabinCols * kCabinRows);
        for (int i = 0; i < count; ++i) {
            const int col = i % kCabinCols;
            const int row = i / kCabinCols;
            const QPointF feet(grid.left() + col * kCellW + 8, grid.top() + (row + 1) * kCellH - 3);
            drawPerson(painter, feet, *people[static_cast<std::size_t>(i)], 0.85);
        }
        const int hidden = static_cast<int>(people.size()) - count;
        if (hidden > 0) {
            drawOverflow(painter, QPointF(cabin.right() - 12, cabin.bottom() - 10), hidden);
        }
        painter.restore();
    }
}

void BuildingView::drawPerson(QPainter& painter, const QPointF& feet, const sim::PersonRow& person, double scale) const
{
    const QColor color = personColor(person.status);
    const double s = scale;
    const QPointF head(feet.x(), feet.y() - 14 * s);
    painter.setPen(QPen(color.darker(140), 1.2));
    painter.setBrush(color);
    painter.drawEllipse(head, 3.8 * s, 3.8 * s);
    painter.drawLine(QPointF(head.x(), head.y() + 3.8 * s), feet);
    painter.drawLine(QPointF(head.x(), head.y() + 6 * s), QPointF(head.x() - 4.5 * s, head.y() + 10 * s));
    painter.drawLine(QPointF(head.x(), head.y() + 6 * s), QPointF(head.x() + 4.5 * s, head.y() + 10 * s));
    painter.drawLine(feet, QPointF(feet.x() - 3.5 * s, feet.y() + 1));
    painter.drawLine(feet, QPointF(feet.x() + 3.5 * s, feet.y() + 1));

    QFont numFont = font();
    numFont.setBold(true);
    numFont.setPointSizeF(std::max(7.0, 8.0 * s));
    painter.setFont(numFont);

    const QString fromText = QString::number(person.origin);
    const QString toText = person.destination > 0 ? QString::number(person.destination) : QStringLiteral("?");
    const QRectF fromBox(head.x() + 5 * s, head.y() - 4 * s, 16, 10 * s);
    const QRectF toBox(head.x() + 5 * s, feet.y() - 10 * s, 16, 10 * s);
    painter.setPen(QColor(90, 90, 100));
    painter.drawText(fromBox, Qt::AlignLeft | Qt::AlignVCenter, fromText);
    painter.setPen(QColor(20, 20, 30));
    painter.drawText(toBox, Qt::AlignLeft | Qt::AlignVCenter, toText);
}

void BuildingView::drawOverflow(QPainter& painter, const QPointF& center, int hidden) const
{
    const QRectF badge(center.x() - 12, center.y() - 8, 24, 16);
    painter.setPen(QPen(QColor(60, 60, 70), 1));
    painter.setBrush(QColor(240, 240, 244));
    painter.drawRoundedRect(badge, 6, 6);
    painter.setPen(QColor(40, 40, 50));
    QFont font = this->font();
    font.setPointSize(7);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(badge, Qt::AlignCenter, QStringLiteral("+%1").arg(hidden));
}
