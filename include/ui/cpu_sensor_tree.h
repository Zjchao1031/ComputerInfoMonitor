#pragma once

#include "model/monitor_snapshot.h"

#include <QHash>
#include <QSet>
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

private:
    struct SensorNodeState
    {
        QTreeWidgetItem* item{nullptr};
        QString valueText;
        bool available{false};
    };

    struct GroupNodeState
    {
        QTreeWidgetItem* item{nullptr};
        QHash<QString, SensorNodeState> sensors;
    };

    [[nodiscard]] GroupNodeState& ensureGroup(const QString& key, const QString& title, int sensorCount, int index);
    [[nodiscard]] SensorNodeState& ensureSensor(GroupNodeState& group, const SensorReading& sensor, int index);
    void updateSensor(SensorNodeState& state, const SensorReading& sensor);
    void removeStaleSensors(GroupNodeState& group, const QSet<QString>& usedSensorIds);
    void removeStaleGroups(const QSet<QString>& usedGroupKeys);

    // 状态中的节点指针均为非拥有观察者，生命周期由 QTreeWidget 管理。
    QHash<QString, GroupNodeState> m_groups;
};

} // namespace monitor

