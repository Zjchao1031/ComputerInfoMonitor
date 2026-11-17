#include "mock/mock_monitor_data.h"

#include <QDateTime>
#include <QStringLiteral>

namespace monitor
{

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

} // namespace monitor

