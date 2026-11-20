#pragma once

#include "model/monitor_snapshot.h"

#include <QWidget>

#include <optional>

class QLabel;

namespace monitor
{

class DetailHeader;
class HistoryChart;

class MemoryDetailPage final : public QWidget
{
    Q_OBJECT

public:
    explicit MemoryDetailPage(QWidget* parent = nullptr);
    ~MemoryDetailPage() override = default;

    void setSnapshot(const MemorySnapshot& snapshot);
    void appendHistorySeries(const QVector<HistorySeries>& histories);

signals:
    void backRequested();

private:
    DetailHeader* m_header{nullptr};
    QLabel* m_memoryValueLabel{nullptr};
    HistoryChart* m_memoryChart{nullptr};
    MemorySnapshot m_snapshot;
    std::optional<double> m_totalMemoryCapacity;
};

} // namespace monitor

