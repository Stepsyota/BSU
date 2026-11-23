#pragma once

#include "Simulation.h"

#include <QPointF>
#include <QWidget>

class QPainter;

class BuildingView : public QWidget {
public:
    explicit BuildingView(QWidget* parent = nullptr);

    void setSnapshot(const sim::SessionSnapshot& snapshot);
    void setSpawnPreview(bool active, int origin, int dest, bool editingOrigin);
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawPerson(QPainter& painter, const QPointF& feet, const sim::PersonRow& person, double scale) const;
    void drawOverflow(QPainter& painter, const QPointF& center, int hidden) const;

    sim::SessionSnapshot snapshot_;
    bool spawnActive_ = false;
    int spawnOrigin_ = 1;
    int spawnDest_ = 2;
    bool spawnEditingOrigin_ = true;
};
