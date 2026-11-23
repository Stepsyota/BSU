#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace sim {

struct ElevatorStats {
    int elevatorId = 0;
    int trips = 0;
    int idleTrips = 0;
    double idleTripPercent = 0.0;
    int transported = 0;
};

struct SessionStats {
    std::vector<ElevatorStats> perElevator;
    int totalTrips = 0;
    int totalTransported = 0;
    double avgWait = 0.0;
    double maxWait = 0.0;
    double sumWait = 0.0;
    int waitSamples = 0;
    int fireCount = 0;
    double fireDuration = 0.0;
};

inline std::string formatReport(const SessionStats& stats)
{
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(2);

    out << "Статистика сеанса симуляции лифтов\n\n";
    for (const auto& row : stats.perElevator) {
        out << "Лифт " << row.elevatorId << ":\n";
        out << "  поездок: " << row.trips << "\n";
        out << "  холостых поездок: " << row.idleTripPercent << " %\n";
        out << "  перевезено людей: " << row.transported << "\n";
    }
    out << "\nВсего поездок: " << stats.totalTrips << "\n";
    out << "Всего перевезено людей: " << stats.totalTransported << "\n";
    out << "Время ожидания: среднее " << stats.avgWait << " с, наибольшее " << stats.maxWait
        << " с, суммарное " << stats.sumWait << " с\n";
    out << "Пожарные тревоги: " << stats.fireCount << ", суммарная длительность " << stats.fireDuration
        << " с\n";
    return out.str();
}

}  // namespace sim
