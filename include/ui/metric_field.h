#pragma once

#include "model/monitor_snapshot.h"

#include <QWidget>

class QLabel;

namespace monitor
{

[[nodiscard]] QString formatMemoryUsage(const MemorySnapshot& snapshot);

class MetricField final : public QWidget
{
public:
    explicit MetricField(const QString& label, QWidget* parent = nullptr);

    void setValue(const MetricValue& metric, int decimalPlaces = 1);

private:
    QLabel* m_valueLabel{nullptr};
    QLabel* m_unitLabel{nullptr};
};

} // namespace monitor

