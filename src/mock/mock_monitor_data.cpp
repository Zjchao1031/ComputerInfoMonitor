#include "mock/mock_monitor_data.h"

#include "model/history_series_ids.h"

#include <QDateTime>
#include <QStringLiteral>

#include <cmath>

namespace monitor
{

namespace
{

HistorySeries makeMockHistorySeries(const char* identifier, const QString& unit, const QDateTime& endTime, double baseValue, double amplitude, double phase, int missingIndex = -1)
{
    HistorySeries history;
    history.identifier = QString::fromLatin1(identifier);
    history.unit = unit;
    history.points.reserve(60);

    for (int index = 0; index < 60; ++index)
    {
        HistoryPoint point;
        point.sampleTime = endTime.addSecs(index - 59);

        if (index == missingIndex)
        {
            point.value = std::nullopt;
        }
        else
        {
            const double wave = std::sin((static_cast<double>(index) + phase) / 5.0);
            point.value = baseValue + amplitude * wave;
        }

        history.points.append(point);
    }

    return history;
}

} // namespace

MonitorSnapshot MockMonitorData::createSnapshot(bool hasDiscreteGpu)
{
    MonitorSnapshot snapshot;
    snapshot.connectionState = ConnectionState::Online;
    snapshot.receivedAt = QDateTime::currentDateTime();
    snapshot.hasDiscreteGpu = hasDiscreteGpu;

    snapshot.cpu.name = QStringLiteral("Intel Core i9-13900K");
    snapshot.cpu.packagePower = MetricValue{65.2, QStringLiteral("W")};
    snapshot.cpu.maximumPCoreClock = MetricValue{3427.2, QStringLiteral("MHz")};
    snapshot.cpu.coreMaximumTemperature = MetricValue{62.5, QStringLiteral("°C")};
    snapshot.cpu.sensorGroups = QVector<SensorGroup>{
        SensorGroup{
            SensorCategory::Voltage,
            QStringLiteral("电压"),
            QVector<SensorReading>{
                SensorReading{QStringLiteral("/intelcpu/0/voltage/0"), QStringLiteral("Voltage"), QStringLiteral("CPU Core"), MetricValue{1.238, QStringLiteral("V")}},
                SensorReading{QStringLiteral("/intelcpu/0/voltage/1"), QStringLiteral("Voltage"), QStringLiteral("VCCIN"), MetricValue{1.792, QStringLiteral("V")}}
            }
        },
        SensorGroup{
            SensorCategory::Clock,
            QStringLiteral("频率"),
            QVector<SensorReading>{
                SensorReading{QStringLiteral("/intelcpu/0/clock/0"), QStringLiteral("Clock"), QStringLiteral("P-Core #1"), MetricValue{3427.2, QStringLiteral("MHz")}},
                SensorReading{QStringLiteral("/intelcpu/0/clock/1"), QStringLiteral("Clock"), QStringLiteral("P-Core #2"), MetricValue{3410.8, QStringLiteral("MHz")}},
                SensorReading{QStringLiteral("/intelcpu/0/clock/8"), QStringLiteral("Clock"), QStringLiteral("E-Core #1"), MetricValue{2890.4, QStringLiteral("MHz")}},
                SensorReading{QStringLiteral("/intelcpu/0/clock/9"), QStringLiteral("Clock"), QStringLiteral("E-Core #2"), MetricValue{2805.2, QStringLiteral("MHz")}}
            }
        },
        SensorGroup{
            SensorCategory::Temperature,
            QStringLiteral("温度"),
            QVector<SensorReading>{
                SensorReading{QStringLiteral("/intelcpu/0/temperature/0"), QStringLiteral("Temperature"), QStringLiteral("P-Core #1"), MetricValue{61.0, QStringLiteral("°C")}},
                SensorReading{QStringLiteral("/intelcpu/0/temperature/1"), QStringLiteral("Temperature"), QStringLiteral("P-Core #2"), MetricValue{62.5, QStringLiteral("°C")}},
                SensorReading{QStringLiteral("/intelcpu/0/temperature/8"), QStringLiteral("Temperature"), QStringLiteral("E-Core #1"), MetricValue{55.0, QStringLiteral("°C")}},
                SensorReading{QStringLiteral("/intelcpu/0/temperature/12"), QStringLiteral("Temperature"), QStringLiteral("Core Max"), MetricValue{62.5, QStringLiteral("°C")}},
                SensorReading{QStringLiteral("/intelcpu/0/temperature/13"), QStringLiteral("Temperature"), QStringLiteral("Core Average"), MetricValue{57.4, QStringLiteral("°C")}},
                SensorReading{QStringLiteral("/intelcpu/0/temperature/14"), QStringLiteral("Temperature"), QStringLiteral("Package"), MetricValue{60.1, QStringLiteral("°C")}}
            }
        }
    };

    snapshot.memory.used = MetricValue{15.3, QStringLiteral("GB")};
    snapshot.memory.total = MetricValue{31.8, QStringLiteral("GB")};

    if (hasDiscreteGpu)
    {
        snapshot.gpu.name = QStringLiteral("NVIDIA GeForce RTX 4070");
        snapshot.gpu.packagePower = MetricValue{120.5, QStringLiteral("W")};
        snapshot.gpu.coreClock = MetricValue{2535.0, QStringLiteral("MHz")};
        snapshot.gpu.coreTemperature = MetricValue{56.0, QStringLiteral("°C")};
        snapshot.gpu.memoryUsed = MetricValue{4096.0, QStringLiteral("MB")};
        snapshot.cpuGpuTotalPower = MetricValue{185.7, QStringLiteral("W")};
    }
    else
    {
        snapshot.cpuGpuTotalPower = snapshot.cpu.packagePower;
    }

    return snapshot;
}

QVector<HistorySeries> MockMonitorData::createHistorySeries(const MonitorSnapshot& snapshot)
{
    QVector<HistorySeries> histories;
    histories.reserve(7);
    histories.append(makeMockHistorySeries(history_id::kCpuPower, QStringLiteral("W"), snapshot.receivedAt, 64.0, 3.8, 0.0));
    histories.append(makeMockHistorySeries(history_id::kCpuClock, QStringLiteral("MHz"), snapshot.receivedAt, 3400.0, 42.0, 1.5));
    histories.append(makeMockHistorySeries(history_id::kCpuTemperature, QStringLiteral("°C"), snapshot.receivedAt, 61.0, 2.8, 3.0, 32));
    histories.append(makeMockHistorySeries(history_id::kGpuClock, QStringLiteral("MHz"), snapshot.receivedAt, 2530.0, 75.0, 2.0));
    histories.append(makeMockHistorySeries(history_id::kGpuTemperature, QStringLiteral("°C"), snapshot.receivedAt, 56.0, 3.2, 4.5));
    histories.append(makeMockHistorySeries(history_id::kGpuMemoryUsed, QStringLiteral("MB"), snapshot.receivedAt, 4088.0, 12.0, 1.0, 41));
    histories.append(makeMockHistorySeries(history_id::kMemoryUsed, QStringLiteral("GB"), snapshot.receivedAt, 15.2, 0.35, 2.5));
    return histories;
}

} // namespace monitor

