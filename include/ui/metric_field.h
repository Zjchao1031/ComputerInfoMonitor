#pragma once

#include "model/monitor_snapshot.h"

#include <QWidget>

class QColor;
class QLabel;

namespace monitor
{

class MetricField final : public QWidget
{
public:
    explicit MetricField(const QString& label, QWidget* parent = nullptr);

    void setValue(const MetricValue& metric, int decimalPlaces = 1);
    void setValueColor(const QColor& color);
    void resetValueColor();

private:
    QLabel* m_valueLabel{nullptr};
    QLabel* m_unitLabel{nullptr};
};

} // namespace monitor

