#pragma once

#include "model/monitor_snapshot.h"

#include <QWidget>

#include <optional>

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
    void appendHistorySeries(const QVector<HistorySeries>& histories);

signals:
    void backRequested();

private:
    DetailHeader* m_header{nullptr};
    MetricField* m_powerField{nullptr};
    MetricField* m_clockField{nullptr};
    MetricField* m_temperatureField{nullptr};
    MetricField* m_memoryField{nullptr};
    HistoryChart* m_powerChart{nullptr};
    HistoryChart* m_clockChart{nullptr};
    HistoryChart* m_temperatureChart{nullptr};
    HistoryChart* m_memoryChart{nullptr};
    GpuSnapshot m_snapshot;
    std::optional<double> m_totalMemoryCapacity;
};

} // namespace monitor

