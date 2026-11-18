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

protected:
    void scrollContentsBy(int dx, int dy) override;
};

} // namespace monitor

