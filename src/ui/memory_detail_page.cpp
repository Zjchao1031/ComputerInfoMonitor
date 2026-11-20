#include "ui/memory_detail_page.h"

#include "model/history_series_ids.h"
#include "ui/detail_header.h"
#include "ui/history_chart.h"
#include "ui/theme.h"

#include <QColor>
#include <QLabel>
#include <QStringLiteral>
#include <QVBoxLayout>

#include <cmath>

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
    m_memoryValueLabel->setText(formatMemoryValue(snapshot));

    if (!m_totalMemoryCapacity.has_value() && snapshot.total.value.has_value() && std::isfinite(*snapshot.total.value) && *snapshot.total.value > 0.0)
    {
        m_totalMemoryCapacity = *snapshot.total.value;
        m_memoryChart->setFixedRange(0.0, *m_totalMemoryCapacity);
    }
}

void MemoryDetailPage::appendHistorySeries(const QVector<HistorySeries>& histories)
{
    const HistorySeries* history = findHistorySeries(histories, history_id::kMemoryUsed);
    if (history != nullptr)
        m_memoryChart->appendData(*history, m_snapshot.used);
}

QString MemoryDetailPage::formatMemoryValue(const MemorySnapshot& snapshot) const
{
    if (!snapshot.used.value.has_value() || !snapshot.total.value.has_value())
        return QStringLiteral("-- / -- GB");

    if (!std::isfinite(*snapshot.used.value) || !std::isfinite(*snapshot.total.value))
        return QStringLiteral("-- / -- GB");

    return QStringLiteral("%1 / %2 GB").arg(QString::number(*snapshot.used.value, 'f', 1), QString::number(*snapshot.total.value, 'f', 1));
}

} // namespace monitor
