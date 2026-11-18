#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVector>

#include <optional>

namespace monitor
{

enum class ConnectionState
{
    Starting,
    Online,
    Delayed,
    Offline,
    NetworkUnavailable,
    ServiceUnavailable,
    ParseError
};

enum class SensorCategory
{
    Voltage,
    Clock,
    Temperature,
    Other
};

struct MetricValue
{
    std::optional<double> value;
    QString unit;

    [[nodiscard]] bool isAvailable() const noexcept
    {
        return value.has_value();
    }
};

struct SensorReading
{
    QString sensorId;
    QString type;
    QString name;
    MetricValue current;
};

struct SensorGroup
{
    SensorCategory category{SensorCategory::Other};
    QString title;
    QVector<SensorReading> sensors;
};

struct CpuSnapshot
{
    QString name;
    MetricValue packagePower;
    MetricValue maximumPCoreClock;
    MetricValue coreMaximumTemperature;
    QVector<SensorGroup> sensorGroups;
};

struct GpuSnapshot
{
    QString name;
    MetricValue packagePower;
    MetricValue coreClock;
    MetricValue coreTemperature;
    MetricValue memoryUsed;
};

struct MemorySnapshot
{
    MetricValue used;
    MetricValue total;
};

struct MonitorSnapshot
{
    ConnectionState connectionState{ConnectionState::Starting};
    QDateTime receivedAt;
    CpuSnapshot cpu;
    GpuSnapshot gpu;
    MemorySnapshot memory;
    MetricValue cpuGpuTotalPower;
    bool hasDiscreteGpu{false};
};

struct HistoryPoint
{
    QDateTime sampleTime;
    std::optional<double> value;
};

struct HistorySeries
{
    QString identifier;
    QString unit;
    int windowSeconds{60};
    int sampleIntervalMs{1000};
    QVector<HistoryPoint> points;
};

} // namespace monitor

Q_DECLARE_METATYPE(monitor::ConnectionState)
Q_DECLARE_METATYPE(monitor::MonitorSnapshot)
Q_DECLARE_METATYPE(monitor::HistorySeries)

