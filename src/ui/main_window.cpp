#include "ui/main_window.h"

#include "ui/cpu_detail_page.h"
#include "ui/dashboard_page.h"
#include "ui/gpu_detail_page.h"
#include "ui/memory_detail_page.h"
#include "ui/theme.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QEvent>
#include <QGuiApplication>
#include <QPalette>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QStackedWidget>
#include <QStringLiteral>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <utility>

namespace monitor
{

namespace
{

class BackgroundWidget final : public QWidget
{
public:
    explicit BackgroundWidget(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_backgroundPixmap(QStringLiteral(":/images/interface-background.png"))
    {
        setObjectName(QStringLiteral("centralWidget"));
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)

        QPainter painter(this);
        painter.drawPixmap(rect(), m_backgroundPixmap);
    }

private:
    QPixmap m_backgroundPixmap;
};

// 背景两侧和底部均有高亮电路图形，内容需要保持在中央安全区内。
constexpr int kSafeAreaLeft = 180;
constexpr int kSafeAreaTop = 29;
constexpr int kSafeAreaRight = 180;
constexpr int kSafeAreaBottom = 82;

QVector<HistorySeries> historySeriesAfter(const QVector<HistorySeries>& histories, const QDateTime& cutoff)
{
    QVector<HistorySeries> selectedHistories;
    selectedHistories.reserve(histories.size());
    for (const HistorySeries& history : histories)
    {
        QVector<HistoryPoint> selectedPoints;
        selectedPoints.reserve(history.points.size());
        for (const HistoryPoint& point : history.points)
        {
            if (point.sampleTime.isValid() && point.sampleTime > cutoff)
                selectedPoints.append(point);
        }

        if (selectedPoints.isEmpty())
            continue;

        HistorySeries selectedHistory = history;
        selectedHistory.points = std::move(selectedPoints);
        selectedHistories.append(std::move(selectedHistory));
    }

    return selectedHistories;
}

QDateTime latestHistorySampleTime(const QVector<HistorySeries>& histories)
{
    QDateTime latestSampleTime;
    for (const HistorySeries& history : histories)
    {
        for (const HistoryPoint& point : history.points)
        {
            if (point.sampleTime.isValid() && (!latestSampleTime.isValid() || point.sampleTime > latestSampleTime))
                latestSampleTime = point.sampleTime;
        }
    }

    return latestSampleTime;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("电脑信息监控终端"));
    setFixedSize(1024, 600);

    auto* centralWidget = new BackgroundWidget(this);

    auto* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(kSafeAreaLeft, kSafeAreaTop, kSafeAreaRight, kSafeAreaBottom);
    rootLayout->setSpacing(0);

    m_pageStack = new QStackedWidget(centralWidget);
    m_pageStack->setObjectName(QStringLiteral("pageStack"));
    m_pageStack->setAttribute(Qt::WA_StyledBackground, true);

    m_dashboardPage = new DashboardPage(m_pageStack);
    m_cpuDetailPage = new CpuDetailPage(m_pageStack);
    m_gpuDetailPage = new GpuDetailPage(m_pageStack);
    m_memoryDetailPage = new MemoryDetailPage(m_pageStack);

    connect(m_dashboardPage, &DashboardPage::cpuRequested, this, &MainWindow::showCpuDetailPage);
    connect(m_dashboardPage, &DashboardPage::gpuRequested, this, &MainWindow::showGpuDetailPage);
    connect(m_dashboardPage, &DashboardPage::memoryRequested, this, &MainWindow::showMemoryDetailPage);
    connect(m_cpuDetailPage, &CpuDetailPage::backRequested, this, &MainWindow::showDashboardPage);
    connect(m_gpuDetailPage, &GpuDetailPage::backRequested, this, &MainWindow::showDashboardPage);
    connect(m_memoryDetailPage, &MemoryDetailPage::backRequested, this, &MainWindow::showDashboardPage);

    m_pageStack->addWidget(m_dashboardPage);
    m_pageStack->addWidget(m_cpuDetailPage);
    m_pageStack->addWidget(m_gpuDetailPage);
    m_pageStack->addWidget(m_memoryDetailPage);
    m_pageStack->setCurrentWidget(m_dashboardPage);
    connect(m_pageStack, &QStackedWidget::currentChanged, this, &MainWindow::handleCurrentPageChanged);
    setPageUpdatesEnabled(m_pageStack->currentIndex());

    rootLayout->addWidget(m_pageStack);
    setCentralWidget(centralWidget);
    setStyleSheet(theme::styleSheet());

    setSnapshot(MonitorSnapshot{});
}

void MainWindow::setSnapshot(const MonitorSnapshot& snapshot)
{
    m_latestSnapshot = snapshot;

    if (!snapshot.hasDiscreteGpu && currentPageId() == PageId::Gpu)
    {
        m_pageStack->setCurrentWidget(m_dashboardPage);
        return;
    }

    markCurrentPageDirty();
    scheduleCurrentPageRefresh();
}

void MainWindow::setHistorySeries(const QVector<HistorySeries>& histories)
{
    const PageId page = currentPageId();
    if (page != PageId::Cpu && page != PageId::Gpu && page != PageId::Memory)
        return;
    if (!m_historyResumeTime.isValid())
        return;

    m_pendingHistories = historySeriesAfter(histories, m_historyResumeTime);
    if (m_pendingHistories.isEmpty())
        return;

    markCurrentPageDirty();
    scheduleCurrentPageRefresh();
}

void MainWindow::prepareForShutdown()
{
    if (m_shutdownPrepared)
        return;

    m_shutdownPrepared = true;

    auto* shutdownOverlay = new QWidget(this);
    shutdownOverlay->setAutoFillBackground(true);

    QPalette overlayPalette = shutdownOverlay->palette();
    overlayPalette.setColor(QPalette::Window, Qt::black);
    shutdownOverlay->setPalette(overlayPalette);
    shutdownOverlay->setGeometry(rect());
    shutdownOverlay->show();
    shutdownOverlay->raise();
    shutdownOverlay->repaint();

    QCoreApplication::sendPostedEvents(shutdownOverlay, QEvent::Paint);
    QGuiApplication::sync();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    event->ignore();
    if (m_shutdownRequested)
        return;

    m_shutdownRequested = true;
    QCoreApplication::quit();
}

void MainWindow::showPage(QWidget* page)
{
    m_pageStack->setCurrentWidget(page);
}

void MainWindow::showDashboardPage()
{
    showPage(m_dashboardPage);
}

void MainWindow::showCpuDetailPage()
{
    showPage(m_cpuDetailPage);
}

void MainWindow::showGpuDetailPage()
{
    showPage(m_gpuDetailPage);
}

void MainWindow::showMemoryDetailPage()
{
    showPage(m_memoryDetailPage);
}

void MainWindow::handleCurrentPageChanged(int index)
{
    setPageUpdatesEnabled(index);
    m_pendingHistories.clear();
    m_historyResumeTime = QDateTime{};

    const PageId page = currentPageId();
    if (page == PageId::Cpu || page == PageId::Gpu || page == PageId::Memory)
    {
        // 以进入详情页时的最新采样时间为基线，避免补绘停留在主页面期间产生的历史点。
        m_historyResumeTime = m_latestSnapshot.receivedAt.isValid() ? m_latestSnapshot.receivedAt : QDateTime::currentDateTime();
    }

    refreshCurrentPage();
}

void MainWindow::setPageUpdatesEnabled(int currentIndex)
{
    for (int index = 0; index < m_pageStack->count(); ++index)
        m_pageStack->widget(index)->setUpdatesEnabled(index == currentIndex);
}

void MainWindow::markCurrentPageDirty()
{
    const PageId page = currentPageId();
    if (page == PageId::Count)
        return;

    m_pageDirty[static_cast<std::size_t>(page)] = true;
}

void MainWindow::scheduleCurrentPageRefresh()
{
    if (m_pageRefreshScheduled)
        return;

    m_pageRefreshScheduled = true;
    QTimer::singleShot(0, this, [this]()
    {
        m_pageRefreshScheduled = false;
        refreshCurrentPageIfDirty();
    });
}

void MainWindow::refreshCurrentPage()
{
    const PageId page = currentPageId();
    if (page == PageId::Count)
        return;

    m_pageDirty[static_cast<std::size_t>(page)] = false;
    refreshPage(page);
}

void MainWindow::refreshCurrentPageIfDirty()
{
    const PageId page = currentPageId();
    if (page == PageId::Count)
        return;

    bool& dirty = m_pageDirty[static_cast<std::size_t>(page)];
    if (!dirty)
        return;

    refreshCurrentPage();
}

void MainWindow::refreshPage(PageId page)
{
    switch (page)
    {
    case PageId::Dashboard:
        m_dashboardPage->setSnapshot(m_latestSnapshot);
        break;
    case PageId::Cpu:
        m_cpuDetailPage->setSnapshot(m_latestSnapshot.cpu);
        if (!m_pendingHistories.isEmpty())
        {
            m_historyResumeTime = latestHistorySampleTime(m_pendingHistories);
            m_cpuDetailPage->appendHistorySeries(m_pendingHistories);
            m_pendingHistories.clear();
        }
        break;
    case PageId::Gpu:
        m_gpuDetailPage->setSnapshot(m_latestSnapshot.gpu);
        if (!m_pendingHistories.isEmpty())
        {
            m_historyResumeTime = latestHistorySampleTime(m_pendingHistories);
            m_gpuDetailPage->appendHistorySeries(m_pendingHistories);
            m_pendingHistories.clear();
        }
        break;
    case PageId::Memory:
        m_memoryDetailPage->setSnapshot(m_latestSnapshot.memory);
        if (!m_pendingHistories.isEmpty())
        {
            m_historyResumeTime = latestHistorySampleTime(m_pendingHistories);
            m_memoryDetailPage->appendHistorySeries(m_pendingHistories);
            m_pendingHistories.clear();
        }
        break;
    case PageId::Count:
        break;
    }
}

MainWindow::PageId MainWindow::currentPageId() const noexcept
{
    const QWidget* currentPage = m_pageStack->currentWidget();
    if (currentPage == m_dashboardPage)
        return PageId::Dashboard;
    if (currentPage == m_cpuDetailPage)
        return PageId::Cpu;
    if (currentPage == m_gpuDetailPage)
        return PageId::Gpu;
    if (currentPage == m_memoryDetailPage)
        return PageId::Memory;

    return PageId::Count;
}

} // namespace monitor
