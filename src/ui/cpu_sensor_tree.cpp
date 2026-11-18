#include "ui/cpu_sensor_tree.h"

#include "ui/theme.h"

#include <QColor>
#include <QFont>
#include <QHash>
#include <QHeaderView>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStringList>
#include <QTimer>
#include <QTreeWidgetItem>

#include <algorithm>
#include <cmath>

namespace monitor
{

namespace
{

bool shouldDisplaySensor(const SensorReading& sensor)
{
    const QString normalizedName = sensor.name.trimmed().toCaseFolded();
    if (normalizedName == QStringLiteral("bus speed") || normalizedName == QStringLiteral("core average"))
        return false;

    return !(sensor.type.trimmed().compare(QStringLiteral("Temperature"), Qt::CaseInsensitive) == 0 && normalizedName == QStringLiteral("cpu package"));
}

struct CoreSortKey
{
    int coreFamily{0};
    int coreIndex{0};
    int variant{0};
};

CoreSortKey coreSortKey(const QString& sensorName)
{
    static const QRegularExpression coreExpression(QStringLiteral(R"(^([PE])-Core\s*#([0-9]+)(?:\s+(.*))?$)"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = coreExpression.match(sensorName.trimmed());
    if (!match.hasMatch())
        return CoreSortKey{};

    CoreSortKey key;
    key.coreFamily = match.captured(1).compare(QStringLiteral("P"), Qt::CaseInsensitive) == 0 ? 1 : 2;

    bool indexIsValid = false;
    key.coreIndex = match.captured(2).toInt(&indexIsValid);
    if (!indexIsValid)
        return CoreSortKey{};

    key.variant = match.captured(3).trimmed().isEmpty() ? 0 : 1;
    return key;
}

bool temperatureSensorLessThan(const SensorReading& left, const SensorReading& right)
{
    const CoreSortKey leftKey = coreSortKey(left.name);
    const CoreSortKey rightKey = coreSortKey(right.name);
    if (leftKey.coreFamily != rightKey.coreFamily)
        return leftKey.coreFamily < rightKey.coreFamily;

    if (leftKey.coreFamily == 0)
        return false;

    if (leftKey.coreIndex != rightKey.coreIndex)
        return leftKey.coreIndex < rightKey.coreIndex;

    return leftKey.variant < rightKey.variant;
}

} // namespace

CpuSensorTree::CpuSensorTree(QWidget* parent)
    : QTreeWidget(parent)
{
    setObjectName(QStringLiteral("sensorTree"));
    setColumnCount(2);
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setIndentation(14);
    setUniformRowHeights(false);
    setAlternatingRowColors(false);
    setFocusPolicy(Qt::NoFocus);
    setSelectionMode(QAbstractItemView::NoSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setWordWrap(true);
    setTextElideMode(Qt::ElideNone);
    setExpandsOnDoubleClick(true);

    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
}

void CpuSensorTree::setGroups(const QVector<SensorGroup>& groups)
{
    QScrollBar* scrollBar = verticalScrollBar();
    const int scrollPosition = scrollBar->value();

    QHash<QString, bool> expansionState;
    for (int index = 0; index < topLevelItemCount(); ++index)
    {
        QTreeWidgetItem* item = topLevelItem(index);
        expansionState.insert(item->text(0), item->isExpanded());
    }

    clear();

    for (const SensorGroup& group : groups)
    {
        QVector<SensorReading> visibleSensors;
        visibleSensors.reserve(group.sensors.size());
        for (const SensorReading& sensor : group.sensors)
        {
            if (shouldDisplaySensor(sensor))
                visibleSensors.append(sensor);
        }

        if (group.category == SensorCategory::Temperature)
            std::stable_sort(visibleSensors.begin(), visibleSensors.end(), temperatureSensorLessThan);

        if (visibleSensors.isEmpty())
            continue;

        auto* groupItem = new QTreeWidgetItem(this, QStringList{QStringLiteral("%1 (%2)").arg(group.title).arg(visibleSensors.size())});
        groupItem->setFirstColumnSpanned(true);

        QFont groupFont = font();
        groupFont.setWeight(QFont::DemiBold);
        groupItem->setFont(0, groupFont);
        groupItem->setForeground(0, QColor(theme::kAccentCyan));

        for (const SensorReading& sensor : visibleSensors)
        {
            QString valueText = QStringLiteral("--");
            if (sensor.current.value.has_value() && std::isfinite(*sensor.current.value))
                valueText = QStringLiteral("%1 %2").arg(QString::number(*sensor.current.value, 'f', 1)).arg(sensor.current.unit);

            auto* sensorItem = new QTreeWidgetItem(groupItem, QStringList{sensor.name, valueText});
            sensorItem->setForeground(0, QColor(theme::kTextPrimary));
            sensorItem->setForeground(1, sensor.current.isAvailable() ? QColor(theme::kTextSecondary) : QColor(theme::kOffline));
            sensorItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        }

        groupItem->setExpanded(expansionState.value(group.title, true));
    }

    QTimer::singleShot(0, this, [scrollBar, scrollPosition]()
    {
        scrollBar->setValue(qMin(scrollPosition, scrollBar->maximum()));
    });
}

void CpuSensorTree::scrollContentsBy(int dx, int dy)
{
    Q_UNUSED(dx)
    QTreeWidget::scrollContentsBy(0, dy);
}

} // namespace monitor
