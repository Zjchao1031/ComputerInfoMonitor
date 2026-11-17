#pragma once

#include "model/monitor_snapshot.h"

#include <QMainWindow>
#include <QVector>

class QStackedWidget;
class QWidget;

namespace monitor
{

class CpuDetailPage;
class DashboardPage;
class GpuDetailPage;
class MemoryDetailPage;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    void setSnapshot(const MonitorSnapshot& snapshot);
    void setHistorySeries(const QVector<HistorySeries>& histories);

private:
    void showPage(QWidget* page);
    void showDashboardPage();
    void showCpuDetailPage();
    void showGpuDetailPage();
    void showMemoryDetailPage();

    QStackedWidget* m_pageStack{nullptr};
    DashboardPage* m_dashboardPage{nullptr};
    CpuDetailPage* m_cpuDetailPage{nullptr};
    GpuDetailPage* m_gpuDetailPage{nullptr};
    MemoryDetailPage* m_memoryDetailPage{nullptr};
};

} // namespace monitor

