#include "ui/history_chart.h"

#include "ui/theme.h"

#include <QDateTime>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QStringLiteral>

#include <algorithm>
#include <cmath>
#include <limits>

namespace monitor
{

namespace
{

bool hasFiniteValue(const std::optional<double>& value)
{
    return value.has_value() && std::isfinite(*value);
}

QString formatValue(double value, int decimalPlaces)
{
    return QString::number(value, 'f', decimalPlaces);
}

} // namespace

HistoryChart::HistoryChart(const QString& title, const QColor& lineColor, int decimalPlaces, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
    , m_lineColor(lineColor)
    , m_decimalPlaces(decimalPlaces)
{
    setMinimumHeight(62);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void HistoryChart::setData(const HistorySeries& history, const MetricValue& current)
{
    m_points = history.points;
    m_current = current;
    m_unit = history.unit.isEmpty() ? current.unit : history.unit;
    m_windowSeconds = history.windowSeconds > 0 ? history.windowSeconds : 60;
    m_sampleIntervalMs = history.sampleIntervalMs > 0 ? history.sampleIntervalMs : 1000;
    update();
}

void HistoryChart::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFont titleFont = font();
    titleFont.setPointSizeF(9.0);
    titleFont.setWeight(QFont::DemiBold);
    painter.setFont(titleFont);
    painter.setPen(QColor(theme::kTextPrimary));
    painter.drawText(QRectF(0.0, 0.0, static_cast<qreal>(width()) * 0.62, 18.0), Qt::AlignLeft | Qt::AlignVCenter, m_title);

    const QString currentText = m_current.isAvailable() ? QStringLiteral("%1 %2").arg(formatValue(*m_current.value, m_decimalPlaces), m_unit) : QStringLiteral("-- %1").arg(m_unit);
    painter.setPen(QColor(theme::kAccentYellow));
    painter.drawText(QRectF(static_cast<qreal>(width()) * 0.45, 0.0, static_cast<qreal>(width()) * 0.55, 18.0), Qt::AlignRight | Qt::AlignVCenter, currentText);

    const QRectF plotRect(44.0, 22.0, static_cast<qreal>(width()) - 52.0, static_cast<qreal>(height()) - 43.0);
    if (plotRect.width() <= 1.0 || plotRect.height() <= 1.0)
        return;

    QDateTime firstTime;
    QDateTime lastTime;

    for (const HistoryPoint& point : m_points)
    {
        if (!point.sampleTime.isValid())
            continue;

        if (!firstTime.isValid())
            firstTime = point.sampleTime;
        lastTime = point.sampleTime;
    }

    if (!firstTime.isValid() || !lastTime.isValid())
    {
        painter.setPen(QColor(theme::kTextSecondary));
        painter.drawText(plotRect, Qt::AlignCenter, QStringLiteral("暂无历史数据"));
        return;
    }

    QDateTime visibleStartTime = firstTime;
    const qint64 windowMilliseconds = static_cast<qint64>(m_windowSeconds) * 1000;
    if (windowMilliseconds > 0)
    {
        const QDateTime windowStartTime = lastTime.addMSecs(-windowMilliseconds);
        if (windowStartTime > visibleStartTime)
            visibleStartTime = windowStartTime;
    }

    const qint64 startMilliseconds = visibleStartTime.toMSecsSinceEpoch();
    qint64 endMilliseconds = lastTime.toMSecsSinceEpoch();
    if (endMilliseconds <= startMilliseconds)
        endMilliseconds = startMilliseconds + 1000;

    bool hasAnyValue = false;
    double minimumValue = 0.0;
    double maximumValue = 0.0;
    for (const HistoryPoint& point : m_points)
    {
        if (!hasFiniteValue(point.value) || !point.sampleTime.isValid())
            continue;

        const qint64 sampleMilliseconds = point.sampleTime.toMSecsSinceEpoch();
        if (sampleMilliseconds < startMilliseconds || sampleMilliseconds > endMilliseconds)
            continue;

        const double value = *point.value;
        if (!hasAnyValue)
        {
            minimumValue = value;
            maximumValue = value;
            hasAnyValue = true;
        }
        else
        {
            minimumValue = std::min(minimumValue, value);
            maximumValue = std::max(maximumValue, value);
        }
    }

    QFont axisFont = font();
    axisFont.setPointSizeF(7.5);
    painter.setFont(axisFont);
    painter.setPen(QPen(QColor(88, 217, 255, 42), 1.0));

    for (int line = 0; line <= 2; ++line)
    {
        const qreal y = plotRect.top() + plotRect.height() * static_cast<qreal>(line) / 2.0;
        painter.drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));
    }

    if (!hasAnyValue)
    {
        painter.setPen(QColor(theme::kTextSecondary));
        painter.drawText(plotRect, Qt::AlignCenter, QStringLiteral("暂无历史数据"));
        return;
    }

    double range = maximumValue - minimumValue;
    if (range <= std::numeric_limits<double>::epsilon())
    {
        const double padding = std::max(1.0, std::abs(maximumValue) * 0.05);
        minimumValue -= padding;
        maximumValue += padding;
        range = maximumValue - minimumValue;
    }
    else
    {
        minimumValue -= range * 0.08;
        maximumValue += range * 0.08;
        range = maximumValue - minimumValue;
    }

    painter.setPen(QColor(theme::kTextSecondary));
    painter.drawText(QRectF(0.0, plotRect.top() - 8.0, 40.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, formatValue(maximumValue, m_decimalPlaces));
    painter.drawText(QRectF(0.0, plotRect.center().y() - 8.0, 40.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, formatValue((minimumValue + maximumValue) / 2.0, m_decimalPlaces));
    painter.drawText(QRectF(0.0, plotRect.bottom() - 8.0, 40.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, formatValue(minimumValue, m_decimalPlaces));

    painter.drawText(QRectF(plotRect.left(), plotRect.bottom() + 2.0, plotRect.width() / 2.0, 16.0), Qt::AlignLeft | Qt::AlignVCenter, visibleStartTime.toString(QStringLiteral("HH:mm:ss")));
    painter.drawText(QRectF(plotRect.center().x(), plotRect.bottom() + 2.0, plotRect.width() / 2.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, lastTime.toString(QStringLiteral("HH:mm:ss")));

    QPainterPath path;
    bool previousPointValid = false;
    qint64 previousSampleMilliseconds = 0;
    const double maximumExpectedGapMilliseconds = static_cast<double>(m_sampleIntervalMs) * 1.5;
    for (const HistoryPoint& point : m_points)
    {
        if (!hasFiniteValue(point.value) || !point.sampleTime.isValid())
        {
            previousPointValid = false;
            continue;
        }

        const qint64 sampleMilliseconds = point.sampleTime.toMSecsSinceEpoch();
        if (sampleMilliseconds < startMilliseconds)
            continue;

        if (previousPointValid && static_cast<double>(sampleMilliseconds - previousSampleMilliseconds) > maximumExpectedGapMilliseconds)
            previousPointValid = false;

        const qreal xRatio = static_cast<qreal>(sampleMilliseconds - startMilliseconds) / static_cast<qreal>(endMilliseconds - startMilliseconds);
        const qreal yRatio = static_cast<qreal>((maximumValue - *point.value) / range);
        const QPointF position(plotRect.left() + xRatio * plotRect.width(), plotRect.top() + yRatio * plotRect.height());

        if (!previousPointValid)
            path.moveTo(position);
        else
            path.lineTo(position);
        previousPointValid = true;
        previousSampleMilliseconds = sampleMilliseconds;
    }

    painter.setPen(QPen(m_lineColor, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path);
}

} // namespace monitor

