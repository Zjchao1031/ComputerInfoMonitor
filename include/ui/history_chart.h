#pragma once

#include "model/monitor_snapshot.h"

#include <QColor>
#include <QWidget>

#include <optional>

class QPaintEvent;
class QShowEvent;

namespace monitor
{

class HistoryChart final : public QWidget
{
public:
    explicit HistoryChart(const QString& title, const QColor& lineColor, int decimalPlaces, QWidget* parent = nullptr);

    void appendData(const HistorySeries& history, const MetricValue& current);
    void setFixedRange(double minimum, double maximum);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    struct AxisRange
    {
        double minimum{0.0};
        double maximum{1.0};
    };

    void trimPoints();

    QString m_title;
    QString m_unit;
    QColor m_lineColor;
    int m_decimalPlaces{1};
    int m_windowSeconds{60};
    int m_sampleIntervalMs{1000};
    qint64 m_timestampOffsetMs{0};
    QVector<HistoryPoint> m_points;
    MetricValue m_current;
    std::optional<AxisRange> m_fixedRange;
};

} // namespace monitor

