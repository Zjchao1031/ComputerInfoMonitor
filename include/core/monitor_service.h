#pragma once

#include "model/monitor_snapshot.h"

#include <QObject>

namespace monitor
{

class MonitorService final : public QObject
{
    Q_OBJECT

public:
    explicit MonitorService(QObject* parent = nullptr);
    ~MonitorService() override = default;

signals:
    void snapshotUpdated(const monitor::MonitorSnapshot& snapshot);
    void connectionStateChanged(monitor::ConnectionState state);
};

} // namespace monitor

