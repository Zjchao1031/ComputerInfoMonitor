#pragma once

#include "model/monitor_snapshot.h"

#include <QDateTime>
#include <QMainWindow>
#include <QVector>

#include <array>
#include <cstddef>

class QStackedWidget;
class QWidget;
class QCloseEvent;

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
    void prepareForShutdown();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    enum class PageId : std::size_t
    {
        Dashboard,
        Cpu,
        Gpu,
        Memory,
        Count
    };

    void showPage(QWidget* page);
    void showDashboardPage();
    void showCpuDetailPage();
    void showGpuDetailPage();
    void showMemoryDetailPage();
    void handleCurrentPageChanged(int index);
    void setPageUpdatesEnabled(int currentIndex);
    void markCurrentPageDirty();
    void scheduleCurrentPageRefresh();
    void refreshCurrentPage();
    void refreshCurrentPageIfDirty();
    void refreshPage(PageId page);
    [[nodiscard]] PageId currentPageId() const noexcept;

    QStackedWidget* m_pageStack{nullptr};
    DashboardPage* m_dashboardPage{nullptr};
    CpuDetailPage* m_cpuDetailPage{nullptr};
    GpuDetailPage* m_gpuDetailPage{nullptr};
    MemoryDetailPage* m_memoryDetailPage{nullptr};
    MonitorSnapshot m_latestSnapshot;
    QVector<HistorySeries> m_pendingHistories;
    QDateTime m_historyResumeTime;
    std::array<bool, static_cast<std::size_t>(PageId::Count)> m_pageDirty{false, false, false, false};
    bool m_pageRefreshScheduled{false};
    bool m_shutdownRequested{false};
    bool m_shutdownPrepared{false};
};

} // namespace monitor

