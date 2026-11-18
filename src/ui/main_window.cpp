#include "ui/main_window.h"

#include "ui/cpu_detail_page.h"
#include "ui/dashboard_page.h"
#include "ui/gpu_detail_page.h"
#include "ui/memory_detail_page.h"
#include "ui/theme.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QPalette>
#include <QStackedWidget>
#include <QStringLiteral>
#include <QVBoxLayout>
#include <QWidget>

namespace monitor
{

namespace
{

// 背景两侧和底部均有高亮电路图形，内容需要保持在中央安全区内。
constexpr int kSafeAreaLeft = 180;
constexpr int kSafeAreaTop = 62;
constexpr int kSafeAreaRight = 180;
constexpr int kSafeAreaBottom = 145;

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("电脑信息监控终端"));
    setFixedSize(1024, 600);

    auto* centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("centralWidget"));
    centralWidget->setAttribute(Qt::WA_StyledBackground, true);

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

    rootLayout->addWidget(m_pageStack);
    setCentralWidget(centralWidget);
    setStyleSheet(theme::styleSheet());

    setSnapshot(MonitorSnapshot{});
}

void MainWindow::setSnapshot(const MonitorSnapshot& snapshot)
{
    m_dashboardPage->setSnapshot(snapshot);
    m_cpuDetailPage->setSnapshot(snapshot.cpu);
    m_gpuDetailPage->setSnapshot(snapshot.gpu);
    m_memoryDetailPage->setSnapshot(snapshot.memory);

    if (!snapshot.hasDiscreteGpu && m_pageStack->currentWidget() == m_gpuDetailPage)
        showPage(m_dashboardPage);
}

void MainWindow::setHistorySeries(const QVector<HistorySeries>& histories)
{
    m_cpuDetailPage->setHistorySeries(histories);
    m_gpuDetailPage->setHistorySeries(histories);
    m_memoryDetailPage->setHistorySeries(histories);
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

} // namespace monitor
