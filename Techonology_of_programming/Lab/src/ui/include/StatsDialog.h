#pragma once

#include "Statistics.h"

#include <QDialog>

class QPlainTextEdit;

class StatsDialog : public QDialog {
    Q_OBJECT

public:
    explicit StatsDialog(sim::SessionStats stats, QWidget* parent = nullptr);

    const sim::SessionStats& stats() const { return stats_; }

private:
    sim::SessionStats stats_;
    QPlainTextEdit* text_ = nullptr;
};
