#pragma once

#include "model/monitor_snapshot.h"

namespace monitor
{

class MockMonitorData final
{
public:
    MockMonitorData() = delete;

    [[nodiscard]] static MonitorSnapshot createSnapshot(bool hasDiscreteGpu);
};

} // namespace monitor

