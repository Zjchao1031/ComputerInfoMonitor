#include "ui/dashboard_page.h"

#include "ui/connection_status_widget.h"
#include "ui/metric_field.h"
#include "ui/monitor_card.h"
#include "ui/theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringLiteral>
#include <QVBoxLayout>

#include <cmath>

namespace monitor
{

DashboardPage::DashboardPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("page"));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(8);

    rootLayout->addWidget(createHeader(this));

    auto* metricsRow = new QHBoxLayout();
    metricsRow->setContentsMargins(0, 0, 0, 0);
    metricsRow->setSpacing(10);
    m_cpuCard = createCpuCard(this);
    m_gpuCard = createGpuCard(this);
    metricsRow->addWidget(m_cpuCard, 1);
    metricsRow->addWidget(m_gpuCard, 1);
    rootLayout->addLayout(metricsRow, 3);

    m_memoryCard = createMemoryCard(this);
    rootLayout->addWidget(m_memoryCard, 1);
    rootLayout->addWidget(createPowerStrip(this));
}

void DashboardPage::setSnapshot(const MonitorSnapshot& snapshot)
{
    m_cpuCard->setHardwareName(snapshot.cpu.name);
    m_cpuPowerField->setValue(snapshot.cpu.packagePower);
    m_cpuClockField->setValue(snapshot.cpu.maximumPCoreClock);
    m_cpuTemperatureField->setValue(snapshot.cpu.coreMaximumTemperature);
    m_cpuTemperatureField->setValueColor(theme::temperatureColor(snapshot.cpu.coreMaximumTemperature));

    m_gpuCard->setHardwareName(snapshot.gpu.name);
    m_gpuPowerField->setValue(snapshot.gpu.packagePower);
    m_gpuClockField->setValue(snapshot.gpu.coreClock);
    m_gpuTemperatureField->setValue(snapshot.gpu.coreTemperature);
    m_gpuMemoryField->setValue(snapshot.gpu.memoryUsed, 0);
    m_gpuCard->setVisible(snapshot.hasDiscreteGpu);

    m_memoryValueLabel->setText(formatMemoryValue(snapshot.memory));

    if (snapshot.cpuGpuTotalPower.value.has_value() && std::isfinite(*snapshot.cpuGpuTotalPower.value))
        m_totalPowerValueLabel->setText(QString::number(*snapshot.cpuGpuTotalPower.value, 'f', 1));
    else
        m_totalPowerValueLabel->setText(QStringLiteral("--"));

    m_totalPowerUnitLabel->setText(snapshot.cpuGpuTotalPower.unit.isEmpty() ? QStringLiteral("W") : snapshot.cpuGpuTotalPower.unit);
    m_connectionStatus->setStatus(snapshot.connectionState, snapshot.receivedAt);
}

QWidget* DashboardPage::createHeader(QWidget* parent)
{
    auto* header = new QWidget(parent);
    header->setObjectName(QStringLiteral("statusHeader"));
    header->setFixedHeight(40);

    auto* layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    auto* titleLabel = new QLabel(QStringLiteral("PC 监控"), header);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));

    m_connectionStatus = new ConnectionStatusWidget(header);

    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(m_connectionStatus);
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

    m_memoryValueLabel = new QLabel(card);
    m_memoryValueLabel->setObjectName(QStringLiteral("memoryValue"));
    m_memoryValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    card->bodyLayout()->addStretch();
    card->bodyLayout()->addWidget(m_memoryValueLabel);
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
