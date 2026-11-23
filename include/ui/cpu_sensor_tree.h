#pragma once

#include "model/monitor_snapshot.h"

#include <QListView>

class QEvent;

namespace monitor
{

class CpuSensorModel;

class CpuSensorTree final : public QListView
{
public:
    explicit CpuSensorTree(QWidget* parent = nullptr);
    ~CpuSensorTree() override = default;

    void setGroups(const QVector<SensorGroup>& groups);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    CpuSensorModel* m_model{nullptr};
    qreal m_lastMouseY{0.0};
    qreal m_pendingDragPixels{0.0};
    bool m_leftButtonPressed{false};
};

} // namespace monitor

