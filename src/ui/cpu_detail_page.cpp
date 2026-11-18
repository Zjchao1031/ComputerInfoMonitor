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
        chart.setData(*history, current);
        return;
    }

    chart.setData(HistorySeries{}, current);
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
    updateCharts();
}

void CpuDetailPage::setHistorySeries(const QVector<HistorySeries>& histories)
{
    m_histories = histories;
    updateCharts();
}

void CpuDetailPage::updateCharts()
{
    updateChart(*m_powerChart, m_histories, history_id::kCpuPower, m_snapshot.packagePower);
    updateChart(*m_clockChart, m_histories, history_id::kCpuClock, m_snapshot.maximumPCoreClock);
    updateChart(*m_temperatureChart, m_histories, history_id::kCpuTemperature, m_snapshot.coreMaximumTemperature);
}

} // namespace monitor

