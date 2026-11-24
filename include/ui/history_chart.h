#pragma once

#include "model/monitor_snapshot.h"

#include <QColor>
#include <QFont>
#include <QPen>
#include <QPixmap>
#include <QSize>
#include <QWidget>

class QEvent;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;

namespace monitor
{

class HistoryChart final : public QWidget
{
public:
    explicit HistoryChart(const QString& title, const QColor& lineColor, int decimalPlaces, QWidget* parent = nullptr);

    void appendData(const HistorySeries& history, const MetricValue& current);
    void appendData(const QVector<HistorySeries>& histories, const char* identifier, const MetricValue& current);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void trimPoints();
    void refreshFonts();
    void invalidateStaticLayer();
    void ensureStaticLayer();

    QString m_title;
    QString m_unit;
    QFont m_titleFont;
    QFont m_axisFont;
    QPen m_gridPen;
    QPen m_curvePen;
    QColor m_titleColor;
    QColor m_currentColor;
    QColor m_axisLabelColor;
    int m_decimalPlaces{1};
    int m_windowSeconds{60};
    int m_sampleIntervalMs{1000};
    qint64 m_timestampOffsetMs{0};
    QVector<HistoryPoint> m_points;
    MetricValue m_current;
    QPixmap m_staticLayer;
    QSize m_staticLayerSize;
    qreal m_staticLayerDevicePixelRatio{1.0};
};

} // namespace monitor

