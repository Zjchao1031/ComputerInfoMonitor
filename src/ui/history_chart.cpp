#include "ui/history_chart.h"

#include "model/history_series_ids.h"
#include "ui/theme.h"

#include <QDateTime>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QShowEvent>
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

QDateTime latestValidSampleTime(const QVector<HistoryPoint>& points)
{
    for (auto iterator = points.crbegin(); iterator != points.crend(); ++iterator)
    {
        if (iterator->sampleTime.isValid())
            return iterator->sampleTime;
    }

    return {};
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

void HistoryChart::appendData(const HistorySeries& history, const MetricValue& current)
{
    m_current = current;
    m_unit = history.unit.isEmpty() ? current.unit : history.unit;
    m_windowSeconds = history.windowSeconds > 0 ? history.windowSeconds : 60;
    m_sampleIntervalMs = history.sampleIntervalMs > 0 ? history.sampleIntervalMs : 1000;

    QDateTime latestSampleTime = latestValidSampleTime(m_points);

    const qint64 sampleIntervalMilliseconds = std::max<qint64>(1, m_sampleIntervalMs);
    const qint64 maximumContinuousGapMilliseconds = sampleIntervalMilliseconds * 3 / 2;
    for (const HistoryPoint& point : history.points)
    {
        if (!point.sampleTime.isValid())
            continue;

        qint64 logicalMilliseconds = point.sampleTime.toMSecsSinceEpoch() + m_timestampOffsetMs;
        if (latestSampleTime.isValid())
        {
            const qint64 latestMilliseconds = latestSampleTime.toMSecsSinceEpoch();
            const qint64 gapMilliseconds = logicalMilliseconds - latestMilliseconds;
            if (gapMilliseconds <= 0)
                continue;

            // 页面隐藏期间不接收采样点，恢复绘制时压缩时间间隔，使曲线从上次位置连续延伸。
            if (gapMilliseconds > maximumContinuousGapMilliseconds)
            {
                m_timestampOffsetMs -= gapMilliseconds - sampleIntervalMilliseconds;
                logicalMilliseconds = point.sampleTime.toMSecsSinceEpoch() + m_timestampOffsetMs;
            }
        }

        HistoryPoint logicalPoint = point;
        logicalPoint.sampleTime = point.sampleTime.addMSecs(m_timestampOffsetMs);
        m_points.append(logicalPoint);
        latestSampleTime = logicalPoint.sampleTime;
    }

    trimPoints();

    if (isVisible())
        update();
}

void HistoryChart::appendData(const QVector<HistorySeries>& histories, const char* identifier, const MetricValue& current)
{
    const HistorySeries* history = findHistorySeries(histories, identifier);
    if (history != nullptr)
        appendData(*history, current);
}

void HistoryChart::setFixedRange(double minimum, double maximum)
{
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum >= maximum)
        return;

    m_fixedRange = AxisRange{minimum, maximum};
    if (isVisible())
        update();
}

void HistoryChart::trimPoints()
{
    const QDateTime latestSampleTime = latestValidSampleTime(m_points);

    if (!latestSampleTime.isValid())
    {
        m_points.clear();
        return;
    }

    const QDateTime windowStartTime = latestSampleTime.addSecs(-m_windowSeconds);
    int removeCount = 0;
    for (const HistoryPoint& point : m_points)
    {
        if (point.sampleTime.isValid() && point.sampleTime >= windowStartTime)
            break;

        ++removeCount;
    }

    if (removeCount > 0)
        m_points.remove(0, removeCount);
}

void HistoryChart::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (!isVisible())
        return;

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

    const QRectF plotRect(44.0, 22.0, static_cast<qreal>(width()) - 52.0, static_cast<qreal>(height()) - 27.0);
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

    qint64 startMilliseconds = 0;
    qint64 endMilliseconds = 1;
    if (firstTime.isValid() && lastTime.isValid())
    {
        QDateTime visibleStartTime = firstTime;
        const qint64 windowMilliseconds = static_cast<qint64>(m_windowSeconds) * 1000;
        if (windowMilliseconds > 0)
        {
            const QDateTime windowStartTime = lastTime.addMSecs(-windowMilliseconds);
            if (windowStartTime > visibleStartTime)
                visibleStartTime = windowStartTime;
        }

        startMilliseconds = visibleStartTime.toMSecsSinceEpoch();
        endMilliseconds = lastTime.toMSecsSinceEpoch();
        if (endMilliseconds <= startMilliseconds)
            endMilliseconds = startMilliseconds + 1000;
    }

    bool hasVisibleValue = false;
    double dataMinimumValue = 0.0;
    double dataMaximumValue = 0.0;
    for (const HistoryPoint& point : m_points)
    {
        if (!hasFiniteValue(point.value) || !point.sampleTime.isValid())
            continue;

        const qint64 sampleMilliseconds = point.sampleTime.toMSecsSinceEpoch();
        if (sampleMilliseconds < startMilliseconds || sampleMilliseconds > endMilliseconds)
            continue;

        const double value = *point.value;
        if (!hasVisibleValue)
        {
            dataMinimumValue = value;
            dataMaximumValue = value;
            hasVisibleValue = true;
        }
        else
        {
            dataMinimumValue = std::min(dataMinimumValue, value);
            dataMaximumValue = std::max(dataMaximumValue, value);
        }
    }

    bool hasAxisRange = false;
    double minimumValue = 0.0;
    double maximumValue = 0.0;
    if (m_fixedRange.has_value())
    {
        minimumValue = m_fixedRange->minimum;
        maximumValue = m_fixedRange->maximum;
        hasAxisRange = true;
    }
    else if (hasVisibleValue)
    {
        minimumValue = dataMinimumValue;
        maximumValue = dataMaximumValue;
        const double range = maximumValue - minimumValue;
        if (range <= std::numeric_limits<double>::epsilon())
        {
            const double padding = std::max(1.0, std::abs(maximumValue) * 0.05);
            minimumValue -= padding;
            maximumValue += padding;
        }
        else
        {
            minimumValue -= range * 0.08;
            maximumValue += range * 0.08;
        }

        hasAxisRange = true;
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

    if (hasAxisRange)
    {
        painter.setPen(QColor(theme::kTextSecondary));
        const double middleValue = (minimumValue + maximumValue) / 2.0;
        painter.drawText(QRectF(0.0, plotRect.top() - 8.0, 40.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, formatValue(maximumValue, m_decimalPlaces));
        painter.drawText(QRectF(0.0, plotRect.center().y() - 8.0, 40.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, formatValue(middleValue, m_decimalPlaces));
        painter.drawText(QRectF(0.0, plotRect.bottom() - 8.0, 40.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, formatValue(minimumValue, m_decimalPlaces));
    }

    if (!firstTime.isValid() || !lastTime.isValid() || !hasAxisRange)
        return;

    const double range = maximumValue - minimumValue;
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
        const double plottedValue = std::clamp(*point.value, minimumValue, maximumValue);
        const qreal yRatio = static_cast<qreal>((maximumValue - plottedValue) / range);
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

void HistoryChart::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    update();
}

} // namespace monitor

