#pragma once

#include "model/monitor_snapshot.h"

#include <QTreeWidget>

namespace monitor
{

class CpuSensorTree final : public QTreeWidget
{
public:
    explicit CpuSensorTree(QWidget* parent = nullptr);

    void setGroups(const QVector<SensorGroup>& groups);
};

} // namespace monitor

