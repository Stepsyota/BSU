#include "JsonStore.h"

#include "Constants.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include <utility>

namespace io {
namespace {

QString readAll(const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("Не удалось открыть файл: %1").arg(path);
        }
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

bool writeAll(const QString& path, const QByteArray& data, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("Не удалось записать файл: %1").arg(path);
        }
        return false;
    }
    file.write(data);
    return true;
}

QJsonDocument parseJson(const QString& path, QString* error)
{
    const QString text = readAll(path, error);
    if (text.isEmpty() && error && !error->isEmpty()) {
        return {};
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = QStringLiteral("Ошибка JSON: %1").arg(parseError.errorString());
        }
        return {};
    }
    return doc;
}

}  // namespace

bool JsonStore::loadConfig(const QString& path, sim::BuildingConfig& config, QString* error)
{
    const QJsonDocument doc = parseJson(path, error);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }
    const QJsonObject root = doc.object();
    sim::BuildingConfig loaded;
    loaded.floorCount = root.value(QStringLiteral("floorCount")).toInt(sim::kDefaultFloorCount);
    loaded.floorHeight = root.value(QStringLiteral("floorHeight")).toDouble(sim::kFloorHeightM);
    const QJsonArray elevators = root.value(QStringLiteral("elevators")).toArray();
    int index = 1;
    for (const auto& item : elevators) {
        const QJsonObject obj = item.toObject();
        sim::ElevatorSpec spec;
        spec.id = obj.value(QStringLiteral("id")).toInt(index);
        spec.capacity = obj.value(QStringLiteral("capacity")).toInt(sim::kDefaultCapacity);
        spec.maxSpeed = obj.value(QStringLiteral("maxSpeed")).toDouble(sim::kDefaultMaxSpeed);
        spec.maxAcceleration = obj.value(QStringLiteral("maxAcceleration")).toDouble(sim::kDefaultMaxAccel);
        loaded.elevators.push_back(spec);
        ++index;
    }
    if (!loaded.isValid()) {
        if (error) {
            *error = QStringLiteral("Конфигурация не проходит ограничения ТЗ");
        }
        return false;
    }
    config = std::move(loaded);
    return true;
}

bool JsonStore::saveConfig(const QString& path, const sim::BuildingConfig& config, QString* error)
{
    QJsonObject root;
    root.insert(QStringLiteral("floorCount"), config.floorCount);
    root.insert(QStringLiteral("floorHeight"), config.floorHeight);
    QJsonArray elevators;
    for (const auto& spec : config.elevators) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), spec.id);
        obj.insert(QStringLiteral("capacity"), spec.capacity);
        obj.insert(QStringLiteral("maxSpeed"), spec.maxSpeed);
        obj.insert(QStringLiteral("maxAcceleration"), spec.maxAcceleration);
        elevators.append(obj);
    }
    root.insert(QStringLiteral("elevators"), elevators);
    return writeAll(path, QJsonDocument(root).toJson(QJsonDocument::Indented), error);
}

bool JsonStore::loadScenario(const QString& path, sim::Scenario& scenario, QString* error)
{
    const QJsonDocument doc = parseJson(path, error);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }
    const QJsonArray events = doc.object().value(QStringLiteral("events")).toArray();
    std::vector<sim::ScenarioEvent> loaded;
    for (const auto& item : events) {
        const QJsonObject obj = item.toObject();
        sim::ScenarioEvent event;
        event.timeSeconds = obj.value(QStringLiteral("time")).toDouble();
        const QString type = obj.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("spawn")) {
            event.type = sim::ScenarioEventType::SpawnPeople;
            event.count = obj.value(QStringLiteral("count")).toInt();
            event.fromFloor = obj.value(QStringLiteral("from")).toInt();
            event.toFloor = obj.value(QStringLiteral("to")).toInt();
        } else if (type == QLatin1String("fireOn")) {
            event.type = sim::ScenarioEventType::FireOn;
        } else if (type == QLatin1String("fireOff")) {
            event.type = sim::ScenarioEventType::FireOff;
        } else {
            if (error) {
                *error = QStringLiteral("Неизвестный тип события: %1").arg(type);
            }
            return false;
        }
        loaded.push_back(event);
    }
    scenario.setEvents(std::move(loaded));
    return true;
}

bool JsonStore::saveScenario(const QString& path, const sim::Scenario& scenario, QString* error)
{
    QJsonArray events;
    for (const auto& event : scenario.events()) {
        QJsonObject obj;
        obj.insert(QStringLiteral("time"), event.timeSeconds);
        switch (event.type) {
        case sim::ScenarioEventType::SpawnPeople:
            obj.insert(QStringLiteral("type"), QStringLiteral("spawn"));
            obj.insert(QStringLiteral("count"), event.count);
            obj.insert(QStringLiteral("from"), event.fromFloor);
            obj.insert(QStringLiteral("to"), event.toFloor);
            break;
        case sim::ScenarioEventType::FireOn:
            obj.insert(QStringLiteral("type"), QStringLiteral("fireOn"));
            break;
        case sim::ScenarioEventType::FireOff:
            obj.insert(QStringLiteral("type"), QStringLiteral("fireOff"));
            break;
        }
        events.append(obj);
    }
    QJsonObject root;
    root.insert(QStringLiteral("events"), events);
    return writeAll(path, QJsonDocument(root).toJson(QJsonDocument::Indented), error);
}

bool JsonStore::saveReport(const QString& path, const sim::SessionStats& stats, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (error) {
            *error = QStringLiteral("Не удалось записать файл: %1").arg(path);
        }
        return false;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << QString::fromStdString(sim::formatReport(stats));
    return true;
}

}  // namespace io
