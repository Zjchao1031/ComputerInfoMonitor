#pragma once

#include "model/monitor_snapshot.h"

#include <QWidget>

namespace monitor
{

class CpuSensorTree;
class DetailHeader;
class HistoryChart;

class CpuDetailPage final : public QWidget
{
    Q_OBJECT

public:
    explicit CpuDetailPage(QWidget* parent = nullptr);
    ~CpuDetailPage() override = default;

    void setSnapshot(const CpuSnapshot& snapshot);
    void appendHistorySeries(const QVector<HistorySeries>& histories);

signals:
    void backRequested();

private:
    void handleSensorScrollingChanged(bool scrolling);

    DetailHeader* m_header{nullptr};
    HistoryChart* m_powerChart{nullptr};
    HistoryChart* m_clockChart{nullptr};
    HistoryChart* m_temperatureChart{nullptr};
    CpuSensorTree* m_sensorTree{nullptr};
    CpuSnapshot m_snapshot;
    bool m_sensorUpdatesPaused{false};
};

} // namespace monitor

