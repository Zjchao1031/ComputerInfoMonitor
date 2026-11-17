#pragma once

#include "model/monitor_snapshot.h"

#include <QMainWindow>

class QLabel;
class QWidget;

namespace monitor
{

class ConnectionStatusWidget;
class MetricField;
class MonitorCard;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    void setSnapshot(const MonitorSnapshot& snapshot);

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
    QLabel* m_memoryValueLabel{nullptr};
    QLabel* m_totalPowerValueLabel{nullptr};
    QLabel* m_totalPowerUnitLabel{nullptr};
};

} // namespace monitor

