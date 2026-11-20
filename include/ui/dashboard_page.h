#pragma once

#include "model/monitor_snapshot.h"

#include <QWidget>

class QLabel;

namespace monitor
{

class ConnectionStatusWidget;
class MetricField;
class MonitorCard;

class DashboardPage final : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget* parent = nullptr);
    ~DashboardPage() override = default;

    void setSnapshot(const MonitorSnapshot& snapshot);

signals:
    void cpuRequested();
    void gpuRequested();
    void memoryRequested();

private:
    [[nodiscard]] QWidget* createHeader(QWidget* parent);
    [[nodiscard]] MonitorCard* createCpuCard(QWidget* parent);
    [[nodiscard]] MonitorCard* createGpuCard(QWidget* parent);
    [[nodiscard]] MonitorCard* createMemoryCard(QWidget* parent);
    [[nodiscard]] QWidget* createPowerStrip(QWidget* parent);
    [[nodiscard]] QString formatMemoryValue(const MemorySnapshot& memory) const;

    ConnectionStatusWidget* m_connectionStatus{nullptr};
    MonitorCard* m_cpuCard{nullptr};
    MonitorCard* m_gpuCard{nullptr};
    MonitorCard* m_memoryCard{nullptr};
    MetricField* m_cpuPowerField{nullptr};
    MetricField* m_cpuClockField{nullptr};
    MetricField* m_cpuTemperatureField{nullptr};
    MetricField* m_gpuPowerField{nullptr};
    MetricField* m_gpuClockField{nullptr};
    MetricField* m_gpuTemperatureField{nullptr};
    MetricField* m_gpuMemoryField{nullptr};
    QLabel* m_lastUpdatedLabel{nullptr};
    QLabel* m_memoryValueLabel{nullptr};
    QLabel* m_totalPowerValueLabel{nullptr};
    QLabel* m_totalPowerUnitLabel{nullptr};
};

} // namespace monitor

