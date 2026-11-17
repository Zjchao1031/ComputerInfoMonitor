#include "ui/metric_field.h"

#include "ui/theme.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QStringLiteral>

#include <cmath>

namespace monitor
{

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
        resetValueColor();
    }
    else
    {
        m_valueLabel->setText(QString::number(*metric.value, 'f', decimalPlaces));
    }

    m_unitLabel->setText(metric.unit);
}

void MetricField::setValueColor(const QColor& color)
{
    m_valueLabel->setStyleSheet(QStringLiteral("color: %1;").arg(color.name()));
}

void MetricField::resetValueColor()
{
    m_valueLabel->setStyleSheet(QString::fromLatin1("color: %1;").arg(QString::fromLatin1(theme::kTextPrimary)));
}

} // namespace monitor

