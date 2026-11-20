#include "ui/gpu_detail_page.h"

#include "model/history_series_ids.h"
#include "ui/detail_header.h"
#include "ui/history_chart.h"
#include "ui/metric_field.h"
#include "ui/theme.h"

#include <QColor>
#include <QGridLayout>
#include <QVBoxLayout>

#include <cmath>

namespace monitor
{

GpuDetailPage::GpuDetailPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("page"));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(4);

    m_header = new DetailHeader(this);
    rootLayout->addWidget(m_header);
    connect(m_header, &DetailHeader::backRequested, this, &GpuDetailPage::backRequested);

    auto* summaryLayout = new QGridLayout();
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    summaryLayout->setHorizontalSpacing(16);
    summaryLayout->setVerticalSpacing(2);
    m_powerField = new MetricField(QStringLiteral("功耗"), this);
    m_clockField = new MetricField(QStringLiteral("GPU Core 频率"), this);
    m_temperatureField = new MetricField(QStringLiteral("GPU Core 温度"), this);
    m_memoryField = new MetricField(QStringLiteral("显存占用"), this);
    summaryLayout->addWidget(m_powerField, 0, 0);
    summaryLayout->addWidget(m_clockField, 0, 1);
    summaryLayout->addWidget(m_temperatureField, 1, 0);
    summaryLayout->addWidget(m_memoryField, 1, 1);
    rootLayout->addLayout(summaryLayout);

    m_powerChart = new HistoryChart(QStringLiteral("GPU Package 功耗"), QColor(theme::kAccentYellow), 1, this);
    m_clockChart = new HistoryChart(QStringLiteral("GPU Core 频率"), QColor(theme::kAccentCyan), 1, this);
    m_temperatureChart = new HistoryChart(QStringLiteral("GPU Core 温度"), QColor(theme::kOffline), 1, this);
    m_memoryChart = new HistoryChart(QStringLiteral("显存占用"), QColor(theme::kAccentYellow), 0, this);
    m_powerChart->setFixedRange(0.0, 800.0);
    m_clockChart->setFixedRange(2000.0, 4000.0);
    rootLayout->addWidget(m_powerChart, 1);
    rootLayout->addWidget(m_clockChart, 1);
    rootLayout->addWidget(m_temperatureChart, 1);
    rootLayout->addWidget(m_memoryChart, 1);
}

void GpuDetailPage::setSnapshot(const GpuSnapshot& snapshot)
{
    m_snapshot = snapshot;
    m_header->setTitle(snapshot.name);
    m_powerField->setValue(snapshot.packagePower);
    m_clockField->setValue(snapshot.coreClock);
    m_temperatureField->setValue(snapshot.coreTemperature);
    m_memoryField->setValue(snapshot.memoryUsed, 0);

    if (!m_totalMemoryCapacity.has_value() && snapshot.memoryTotal.value.has_value() && std::isfinite(*snapshot.memoryTotal.value) && *snapshot.memoryTotal.value > 0.0)
    {
        m_totalMemoryCapacity = *snapshot.memoryTotal.value;
        m_memoryChart->setFixedRange(0.0, *m_totalMemoryCapacity);
    }
}

void GpuDetailPage::appendHistorySeries(const QVector<HistorySeries>& histories)
{
    m_powerChart->appendData(histories, history_id::kGpuPower, m_snapshot.packagePower);
    m_clockChart->appendData(histories, history_id::kGpuClock, m_snapshot.coreClock);
    m_temperatureChart->appendData(histories, history_id::kGpuTemperature, m_snapshot.coreTemperature);
    m_memoryChart->appendData(histories, history_id::kGpuMemoryUsed, m_snapshot.memoryUsed);
}

} // namespace monitor

