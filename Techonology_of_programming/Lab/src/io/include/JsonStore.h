#pragma once

#include "BuildingConfig.h"
#include "Scenario.h"
#include "Statistics.h"

#include <QString>

namespace io {

class JsonStore {
public:
    static bool loadConfig(const QString& path, sim::BuildingConfig& config, QString* error = nullptr);
    static bool saveConfig(const QString& path, const sim::BuildingConfig& config, QString* error = nullptr);

    static bool loadScenario(const QString& path, sim::Scenario& scenario, QString* error = nullptr);
    static bool saveScenario(const QString& path, const sim::Scenario& scenario, QString* error = nullptr);

    static bool saveReport(const QString& path, const sim::SessionStats& stats, QString* error = nullptr);
};

}  // namespace io
