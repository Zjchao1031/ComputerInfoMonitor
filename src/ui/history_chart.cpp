#include "ui/history_chart.h"

#include "model/history_series_ids.h"
#include "ui/theme.h"

#include <QDateTime>
#include <QEvent>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStringLiteral>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>

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

QRectF plotRectFor(const QSize& size)
{
    return QRectF(44.0, 22.0, static_cast<qreal>(size.width()) - 52.0, static_cast<qreal>(size.height()) - 27.0);
}

} // namespace

HistoryChart::HistoryChart(const QString& title, const QColor& lineColor, int decimalPlaces, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
    , m_gridPen(QColor(88, 217, 255, 42), 1.0)
    , m_curvePen(lineColor, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , m_titleColor(QColor(theme::kTextPrimary))
    , m_currentColor(QColor(theme::kAccentYellow))
    , m_axisLabelColor(QColor(theme::kTextSecondary))
    , m_decimalPlaces(decimalPlaces)
{
    refreshFonts();
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

void HistoryChart::refreshFonts()
{
    m_titleFont = font();
    m_titleFont.setPointSizeF(9.0);
    m_titleFont.setWeight(QFont::DemiBold);

    m_axisFont = font();
    m_axisFont.setPointSizeF(7.5);
}

void HistoryChart::invalidateStaticLayer()
{
    m_staticLayer = QPixmap{};
    m_staticLayerSize = QSize{};
    m_staticLayerDevicePixelRatio = 1.0;
}

void HistoryChart::ensureStaticLayer()
{
    const qreal devicePixelRatio = devicePixelRatioF();
    if (!m_staticLayer.isNull() && m_staticLayerSize == size() && m_staticLayerDevicePixelRatio == devicePixelRatio)
        return;

    const qreal validDevicePixelRatio = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
    const int pixmapWidth = std::max(1, static_cast<int>(std::ceil(static_cast<qreal>(width()) * validDevicePixelRatio)));
    const int pixmapHeight = std::max(1, static_cast<int>(std::ceil(static_cast<qreal>(height()) * validDevicePixelRatio)));

    QPixmap staticLayer(pixmapWidth, pixmapHeight);
    staticLayer.setDevicePixelRatio(validDevicePixelRatio);
    staticLayer.fill(Qt::transparent);

    QPainter painter(&staticLayer);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(m_titleFont);
    painter.setPen(m_titleColor);
    painter.drawText(QRectF(0.0, 0.0, static_cast<qreal>(width()) * 0.62, 18.0), Qt::AlignLeft | Qt::AlignVCenter, m_title);

    const QRectF plotRect = plotRectFor(size());
    if (plotRect.width() > 1.0 && plotRect.height() > 1.0)
    {
        painter.setPen(m_gridPen);
        for (int line = 0; line <= 2; ++line)
        {
            const qreal y = plotRect.top() + plotRect.height() * static_cast<qreal>(line) / 2.0;
            painter.drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));
        }
    }

    m_staticLayer = std::move(staticLayer);
    m_staticLayerSize = size();
    m_staticLayerDevicePixelRatio = validDevicePixelRatio;
}

void HistoryChart::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (!isVisible())
        return;

    ensureStaticLayer();

    QPainter painter(this);
    if (!m_staticLayer.isNull())
        painter.drawPixmap(QPoint(0, 0), m_staticLayer);

    painter.setRenderHint(QPainter::Antialiasing, true);

    const QString currentText = m_current.isAvailable() ? QStringLiteral("%1 %2").arg(formatValue(*m_current.value, m_decimalPlaces), m_unit) : QStringLiteral("-- %1").arg(m_unit);
    painter.setFont(m_titleFont);
    painter.setPen(m_currentColor);
    painter.drawText(QRectF(static_cast<qreal>(width()) * 0.45, 0.0, static_cast<qreal>(width()) * 0.55, 18.0), Qt::AlignRight | Qt::AlignVCenter, currentText);

    const QRectF plotRect = plotRectFor(size());
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

    double minimumValue = 0.0;
    double maximumValue = 0.0;
    if (hasVisibleValue)
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
    }

    painter.setFont(m_axisFont);

    if (hasVisibleValue)
    {
        painter.setPen(m_axisLabelColor);
        const double middleValue = (minimumValue + maximumValue) / 2.0;
        painter.drawText(QRectF(0.0, plotRect.top() - 8.0, 40.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, formatValue(maximumValue, m_decimalPlaces));
        painter.drawText(QRectF(0.0, plotRect.center().y() - 8.0, 40.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, formatValue(middleValue, m_decimalPlaces));
        painter.drawText(QRectF(0.0, plotRect.bottom() - 8.0, 40.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, formatValue(minimumValue, m_decimalPlaces));
    }

    if (!firstTime.isValid() || !lastTime.isValid() || !hasVisibleValue)
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

    painter.setPen(m_curvePen);
    painter.drawPath(path);
}

void HistoryChart::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    invalidateStaticLayer();
}

void HistoryChart::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);

    if (event == nullptr)
        return;

    if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange)
        refreshFonts();

    if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange || event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        invalidateStaticLayer();
}

void HistoryChart::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    update();
}

} // namespace monitor

