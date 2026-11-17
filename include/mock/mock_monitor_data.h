#pragma once

#include "model/monitor_snapshot.h"

#include <QVector>

namespace monitor
{

class MockMonitorData final
{
public:
    MockMonitorData() = delete;

    [[nodiscard]] static MonitorSnapshot createSnapshot(bool hasDiscreteGpu);
    [[nodiscard]] static QVector<HistorySeries> createHistorySeries(const MonitorSnapshot& snapshot);
};

} // namespace monitor

