#include "ui/memory_detail_page.h"

#include "model/history_series_ids.h"
#include "ui/detail_header.h"
#include "ui/history_chart.h"
#include "ui/metric_field.h"
#include "ui/theme.h"

#include <QColor>
#include <QLabel>
#include <QStringLiteral>
#include <QVBoxLayout>

namespace monitor
{

MemoryDetailPage::MemoryDetailPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("page"));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(4);

    m_header = new DetailHeader(this);
    m_header->setTitle(QStringLiteral("内存"));
    rootLayout->addWidget(m_header);
    connect(m_header, &DetailHeader::backRequested, this, &MemoryDetailPage::backRequested);

    m_memoryValueLabel = new QLabel(this);
    m_memoryValueLabel->setObjectName(QStringLiteral("memoryDetailValue"));
    m_memoryValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    rootLayout->addWidget(m_memoryValueLabel);

    m_memoryChart = new HistoryChart(QStringLiteral("内存已使用容量"), QColor(theme::kAccentYellow), 1, this);
    rootLayout->addWidget(m_memoryChart, 1);
}

void MemoryDetailPage::setSnapshot(const MemorySnapshot& snapshot)
{
    m_snapshot = snapshot;
    m_memoryValueLabel->setText(formatMemoryUsage(snapshot));
}

void MemoryDetailPage::appendHistorySeries(const QVector<HistorySeries>& histories)
{
    const HistorySeries* history = findHistorySeries(histories, history_id::kMemoryUsed);
    if (history != nullptr)
        m_memoryChart->appendData(*history, m_snapshot.used);
}

} // namespace monitor
