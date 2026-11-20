#include "ui/metric_field.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QStringLiteral>

#include <cmath>

namespace monitor
{

QString formatMemoryUsage(const MemorySnapshot& snapshot)
{
    if (!snapshot.used.value.has_value() || !snapshot.total.value.has_value())
        return QStringLiteral("-- / -- GB");

    if (!std::isfinite(*snapshot.used.value) || !std::isfinite(*snapshot.total.value))
        return QStringLiteral("-- / -- GB");

    return QStringLiteral("%1 / %2 GB").arg(QString::number(*snapshot.used.value, 'f', 1), QString::number(*snapshot.total.value, 'f', 1));
}

MetricField::MetricField(const QString& label, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* nameLabel = new QLabel(label, this);
    nameLabel->setObjectName(QStringLiteral("metricLabel"));

    m_valueLabel = new QLabel(QStringLiteral("--"), this);
    m_valueLabel->setObjectName(QStringLiteral("metricValue"));
    m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_valueLabel->setMinimumWidth(108);
    m_valueLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_unitLabel = new QLabel(this);
    m_unitLabel->setObjectName(QStringLiteral("metricUnit"));
    m_unitLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_unitLabel->setMinimumWidth(42);

    layout->addWidget(nameLabel);
    layout->addStretch();
    layout->addWidget(m_valueLabel);
    layout->addSpacing(8);
    layout->addWidget(m_unitLabel);
}

void MetricField::setValue(const MetricValue& metric, int decimalPlaces)
{
    if (!metric.value.has_value() || !std::isfinite(*metric.value))
    {
        m_valueLabel->setText(QStringLiteral("--"));
    }
    else
    {
        m_valueLabel->setText(QString::number(*metric.value, 'f', decimalPlaces));
    }

    m_unitLabel->setText(metric.unit);
}

} // namespace monitor

