#include "ui/dashboard_page.h"

#include "ui/connection_status_widget.h"
#include "ui/metric_field.h"
#include "ui/monitor_card.h"
#include "ui/theme.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringLiteral>
#include <QVBoxLayout>

#include <cmath>

namespace monitor
{

namespace
{

constexpr int kHardwareCardsHeight = 212;
constexpr int kMemoryCardHeight = 130;

} // namespace

DashboardPage::DashboardPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("page"));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(8);

    rootLayout->addWidget(createHeader(this));

    auto* metricsContainer = new QWidget(this);
    metricsContainer->setFixedHeight(kHardwareCardsHeight);

    auto* metricsRow = new QHBoxLayout(metricsContainer);
    metricsRow->setContentsMargins(0, 0, 0, 0);
    metricsRow->setSpacing(10);
    m_cpuCard = createCpuCard(this);
    m_gpuCard = createGpuCard(this);
    metricsRow->addWidget(m_cpuCard, 1);
    metricsRow->addWidget(m_gpuCard, 1);
    rootLayout->addWidget(metricsContainer);

    m_memoryCard = createMemoryCard(this);
    m_memoryCard->setFixedHeight(kMemoryCardHeight);
    rootLayout->addWidget(m_memoryCard);
    rootLayout->addWidget(createPowerStrip(this));
    rootLayout->addStretch();
}

void DashboardPage::setSnapshot(const MonitorSnapshot& snapshot)
{
    m_cpuCard->setHardwareName(snapshot.cpu.name);
    m_cpuPowerField->setValue(snapshot.cpu.packagePower);
    m_cpuClockField->setValue(snapshot.cpu.maximumPCoreClock);
    m_cpuTemperatureField->setValue(snapshot.cpu.coreMaximumTemperature);

    const GpuSnapshot gpuSnapshot = snapshot.hasDiscreteGpu ? snapshot.gpu : GpuSnapshot{};
    m_gpuCard->setHardwareName(gpuSnapshot.name);
    m_gpuPowerField->setValue(gpuSnapshot.packagePower);
    m_gpuClockField->setValue(gpuSnapshot.coreClock);
    m_gpuTemperatureField->setValue(gpuSnapshot.coreTemperature);
    m_gpuMemoryField->setValue(gpuSnapshot.memoryUsed, 0);

    m_memoryValueLabel->setText(formatMemoryValue(snapshot.memory));

    if (snapshot.cpuGpuTotalPower.value.has_value() && std::isfinite(*snapshot.cpuGpuTotalPower.value))
        m_totalPowerValueLabel->setText(QString::number(*snapshot.cpuGpuTotalPower.value, 'f', 1));
    else
        m_totalPowerValueLabel->setText(QStringLiteral("--"));

    m_totalPowerUnitLabel->setText(snapshot.cpuGpuTotalPower.unit.isEmpty() ? QStringLiteral("W") : snapshot.cpuGpuTotalPower.unit);
    m_connectionStatus->setStatus(snapshot.connectionState);

    if (snapshot.receivedAt.isValid())
        m_lastUpdatedLabel->setText(QStringLiteral("最后更新 %1").arg(snapshot.receivedAt.toString(QStringLiteral("HH:mm:ss"))));
    else
        m_lastUpdatedLabel->setText(QStringLiteral("暂无更新时间"));
}

QWidget* DashboardPage::createHeader(QWidget* parent)
{
    auto* header = new QWidget(parent);
    header->setObjectName(QStringLiteral("statusHeader"));
    header->setFixedHeight(40);

    auto* layout = new QGridLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* leftContainer = new QWidget(header);
    auto* leftLayout = new QHBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(16);

    auto* titleLabel = new QLabel(QStringLiteral("PC 监控"), leftContainer);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));

    m_lastUpdatedLabel = new QLabel(leftContainer);
    m_lastUpdatedLabel->setObjectName(QStringLiteral("lastUpdated"));
    m_lastUpdatedLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    leftLayout->addWidget(titleLabel);
    leftLayout->addWidget(m_lastUpdatedLabel);
    leftLayout->addStretch();

    m_connectionStatus = new ConnectionStatusWidget(header);

    auto* rightSpacer = new QWidget(header);
    layout->addWidget(leftContainer, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(m_connectionStatus, 0, 1, Qt::AlignCenter);
    layout->addWidget(rightSpacer, 0, 2);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(2, 1);
    return header;
}

MonitorCard* DashboardPage::createCpuCard(QWidget* parent)
{
    auto* card = new MonitorCard(QStringLiteral("CPU"), parent);
    card->setInteractive(true);

    m_cpuPowerField = new MetricField(QStringLiteral("功耗"), card);
    m_cpuClockField = new MetricField(QStringLiteral("P-Core 最高频率"), card);
    m_cpuTemperatureField = new MetricField(QStringLiteral("Core Max 温度"), card);

    card->bodyLayout()->addWidget(m_cpuPowerField);
    card->bodyLayout()->addWidget(m_cpuClockField);
    card->bodyLayout()->addWidget(m_cpuTemperatureField);
    card->bodyLayout()->addStretch();
    connect(card, &MonitorCard::activated, this, &DashboardPage::cpuRequested);
    return card;
}

MonitorCard* DashboardPage::createGpuCard(QWidget* parent)
{
    auto* card = new MonitorCard(QStringLiteral("GPU"), parent);
    card->setInteractive(true);

    m_gpuPowerField = new MetricField(QStringLiteral("功耗"), card);
    m_gpuClockField = new MetricField(QStringLiteral("GPU Core 频率"), card);
    m_gpuTemperatureField = new MetricField(QStringLiteral("GPU Core 温度"), card);
    m_gpuMemoryField = new MetricField(QStringLiteral("显存占用"), card);

    card->bodyLayout()->addWidget(m_gpuPowerField);
    card->bodyLayout()->addWidget(m_gpuClockField);
    card->bodyLayout()->addWidget(m_gpuTemperatureField);
    card->bodyLayout()->addWidget(m_gpuMemoryField);
    card->bodyLayout()->addStretch();
    connect(card, &MonitorCard::activated, this, &DashboardPage::gpuRequested);
    return card;
}

MonitorCard* DashboardPage::createMemoryCard(QWidget* parent)
{
    auto* card = new MonitorCard(QStringLiteral("内存"), parent);
    card->setInteractive(true);

    auto* memoryLayout = new QHBoxLayout();
    memoryLayout->setContentsMargins(0, 0, 0, 0);
    memoryLayout->setSpacing(12);

    auto* memoryLabel = new QLabel(QStringLiteral("已使用/总容量"), card);
    memoryLabel->setObjectName(QStringLiteral("metricLabel"));
    memoryLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_memoryValueLabel = new QLabel(card);
    m_memoryValueLabel->setObjectName(QStringLiteral("memoryValue"));
    m_memoryValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    memoryLayout->addWidget(memoryLabel);
    memoryLayout->addStretch();
    memoryLayout->addWidget(m_memoryValueLabel);

    card->bodyLayout()->addLayout(memoryLayout);
    card->bodyLayout()->addStretch();
    connect(card, &MonitorCard::activated, this, &DashboardPage::memoryRequested);
    return card;
}

QWidget* DashboardPage::createPowerStrip(QWidget* parent)
{
    auto* powerStrip = new QFrame(parent);
    powerStrip->setObjectName(QStringLiteral("powerStrip"));
    powerStrip->setFixedHeight(48);

    auto* layout = new QHBoxLayout(powerStrip);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(8);

    auto* titleLabel = new QLabel(QStringLiteral("CPU+GPU 功耗"), powerStrip);
    titleLabel->setObjectName(QStringLiteral("powerTitle"));

    m_totalPowerValueLabel = new QLabel(QStringLiteral("--"), powerStrip);
    m_totalPowerValueLabel->setObjectName(QStringLiteral("powerValue"));
    m_totalPowerValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_totalPowerUnitLabel = new QLabel(QStringLiteral("W"), powerStrip);
    m_totalPowerUnitLabel->setObjectName(QStringLiteral("powerUnit"));
    m_totalPowerUnitLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(m_totalPowerValueLabel);
    layout->addWidget(m_totalPowerUnitLabel);
    return powerStrip;
}

QString DashboardPage::formatMemoryValue(const MemorySnapshot& memory) const
{
    if (!memory.used.value.has_value() || !memory.total.value.has_value())
        return QStringLiteral("-- / -- GB");

    if (!std::isfinite(*memory.used.value) || !std::isfinite(*memory.total.value))
        return QStringLiteral("-- / -- GB");

    return QStringLiteral("%1 / %2 GB").arg(QString::number(*memory.used.value, 'f', 1), QString::number(*memory.total.value, 'f', 1));
}

} // namespace monitor
