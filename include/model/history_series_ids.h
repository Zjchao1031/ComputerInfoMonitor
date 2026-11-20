#pragma once

#include "model/monitor_snapshot.h"

#include <QString>

namespace monitor::history_id
{

inline constexpr const char* kCpuPower = "cpu.package_power";
inline constexpr const char* kCpuClock = "cpu.p_core_max_clock";
inline constexpr const char* kCpuTemperature = "cpu.core_max_temperature";
inline constexpr const char* kGpuPower = "gpu.package_power";
inline constexpr const char* kGpuClock = "gpu.core_clock";
inline constexpr const char* kGpuTemperature = "gpu.core_temperature";
inline constexpr const char* kGpuMemoryUsed = "gpu.memory_used";
inline constexpr const char* kMemoryUsed = "memory.used";

} // namespace monitor::history_id

namespace monitor
{

inline const HistorySeries* findHistorySeries(const QVector<HistorySeries>& histories, const char* identifier)
{
    const QString requestedIdentifier = QString::fromLatin1(identifier);
    for (const HistorySeries& history : histories)
    {
        if (history.identifier == requestedIdentifier)
            return &history;
    }

    return nullptr;
}

} // namespace monitor

