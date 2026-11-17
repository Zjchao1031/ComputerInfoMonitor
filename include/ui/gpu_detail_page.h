#pragma once

#include "model/monitor_snapshot.h"

#include <QWidget>

namespace monitor
{

class DetailHeader;
class HistoryChart;
class MetricField;

class GpuDetailPage final : public QWidget
{
    Q_OBJECT

public:
    explicit GpuDetailPage(QWidget* parent = nullptr);
    ~GpuDetailPage() override = default;

    void setSnapshot(const GpuSnapshot& snapshot);
    void setHistorySeries(const QVector<HistorySeries>& histories);

signals:
    void backRequested();

private:
    void updateCharts();

    DetailHeader* m_header{nullptr};
    MetricField* m_powerField{nullptr};
    MetricField* m_clockField{nullptr};
    MetricField* m_temperatureField{nullptr};
    MetricField* m_memoryField{nullptr};
    HistoryChart* m_clockChart{nullptr};
    HistoryChart* m_temperatureChart{nullptr};
    HistoryChart* m_memoryChart{nullptr};
    GpuSnapshot m_snapshot;
    QVector<HistorySeries> m_histories;
};

} // namespace monitor

