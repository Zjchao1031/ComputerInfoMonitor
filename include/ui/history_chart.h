#pragma once

#include "model/monitor_snapshot.h"

#include <QColor>
#include <QWidget>

class QPaintEvent;

namespace monitor
{

class HistoryChart final : public QWidget
{
public:
    explicit HistoryChart(const QString& title, const QColor& lineColor, int decimalPlaces, QWidget* parent = nullptr);

    void setData(const HistorySeries& history, const MetricValue& current);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_title;
    QString m_unit;
    QColor m_lineColor;
    int m_decimalPlaces{1};
    QVector<HistoryPoint> m_points;
    MetricValue m_current;
};

} // namespace monitor

