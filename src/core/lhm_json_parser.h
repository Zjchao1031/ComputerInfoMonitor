#pragma once

#include "model/monitor_snapshot.h"

#include <QByteArray>
#include <QString>

#include <optional>

namespace monitor::detail
{

struct LhmParseResult
{
    std::optional<MonitorSnapshot> snapshot;
    QString errorMessage;
};

[[nodiscard]] LhmParseResult parseLhmResponse(const QByteArray& payload);

} // namespace monitor::detail
