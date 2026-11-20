#include "ui/cpu_detail_page.h"

#include "model/history_series_ids.h"
#include "ui/cpu_sensor_tree.h"
#include "ui/detail_header.h"
#include "ui/history_chart.h"
#include "ui/theme.h"

#include <QColor>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace monitor
{

namespace
{

void updateChart(HistoryChart& chart, const QVector<HistorySeries>& histories, const char* identifier, const MetricValue& current)
{
    const HistorySeries* history = findHistorySeries(histories, identifier);
    if (history != nullptr)
    {
        chart.appendData(*history, current);
    }
}

} // namespace

CpuDetailPage::CpuDetailPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("page"));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(4);

    m_header = new DetailHeader(this);
    rootLayout->addWidget(m_header);
    connect(m_header, &DetailHeader::backRequested, this, &CpuDetailPage::backRequested);

    auto* bodyLayout = new QHBoxLayout();
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(10);

    auto* chartLayout = new QVBoxLayout();
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->setSpacing(4);
    m_powerChart = new HistoryChart(QStringLiteral("CPU Package 功耗"), QColor(theme::kAccentYellow), 1, this);
    m_clockChart = new HistoryChart(QStringLiteral("P-Core 最高频率"), QColor(theme::kAccentCyan), 1, this);
    m_temperatureChart = new HistoryChart(QStringLiteral("Core Max 温度"), QColor(theme::kOffline), 1, this);
    m_powerChart->setFixedRange(0.0, 300.0);
    m_clockChart->setFixedRange(2000.0, 6000.0);
    m_temperatureChart->setFixedRange(30.0, 110.0);
    chartLayout->addWidget(m_powerChart);
    chartLayout->addWidget(m_clockChart);
    chartLayout->addWidget(m_temperatureChart);

    m_sensorTree = new CpuSensorTree(this);

    bodyLayout->addLayout(chartLayout, 7);
    bodyLayout->addWidget(m_sensorTree, 3);
    rootLayout->addLayout(bodyLayout, 1);
}

void CpuDetailPage::setSnapshot(const CpuSnapshot& snapshot)
{
    m_snapshot = snapshot;
    m_header->setTitle(snapshot.name);
    m_sensorTree->setGroups(snapshot.sensorGroups);
}

void CpuDetailPage::appendHistorySeries(const QVector<HistorySeries>& histories)
{
    updateChart(*m_powerChart, histories, history_id::kCpuPower, m_snapshot.packagePower);
    updateChart(*m_clockChart, histories, history_id::kCpuClock, m_snapshot.maximumPCoreClock);
    updateChart(*m_temperatureChart, histories, history_id::kCpuTemperature, m_snapshot.coreMaximumTemperature);
}

} // namespace monitor

