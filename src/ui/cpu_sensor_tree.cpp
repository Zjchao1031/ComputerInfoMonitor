#include "ui/cpu_sensor_tree.h"

#include "ui/theme.h"

#include <QColor>
#include <QFont>
#include <QHash>
#include <QHeaderView>
#include <QRegularExpression>
#include <QScroller>
#include <QSet>
#include <QStringList>
#include <QTreeWidgetItem>

#include <algorithm>
#include <cmath>
#include <memory>

namespace monitor
{

namespace
{

constexpr int kValueColumnWidth = 76;

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

QString groupKey(SensorCategory category, const QString& title)
{
    // 分隔符不会出现在当前显示标题中，避免不同类别和标题组合产生相同键。
    return QString::number(static_cast<int>(category)) + QChar(0x1f) + title;
}

QString sensorValueText(const SensorReading& sensor)
{
    if (sensor.current.value.has_value() && std::isfinite(*sensor.current.value))
        return QStringLiteral("%1 %2").arg(QString::number(*sensor.current.value, 'f', 1)).arg(sensor.current.unit);

    return QStringLiteral("--");
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
    setUniformRowHeights(true);
    setAlternatingRowColors(false);
    setFocusPolicy(Qt::NoFocus);
    setSelectionMode(QAbstractItemView::NoSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setWordWrap(false);
    setTextElideMode(Qt::ElideRight);
    setExpandsOnDoubleClick(true);

    // tslib 将触摸转换为左键拖动，列表需要按鼠标拖动手势接管平移。
    QScroller::grabGesture(viewport(), QScroller::LeftMouseButtonGesture);

    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::Fixed);
    header()->resizeSection(1, kValueColumnWidth);
    header()->setStretchLastSection(false);
}

void CpuSensorTree::setGroups(const QVector<SensorGroup>& groups)
{
    QSet<QString> usedGroupKeys;
    int visibleGroupIndex = 0;
    for (const SensorGroup& group : groups)
    {
        QVector<const SensorReading*> visibleSensors;
        visibleSensors.reserve(group.sensors.size());
        for (const SensorReading& sensor : group.sensors)
        {
            if (shouldDisplaySensor(sensor))
                visibleSensors.append(&sensor);
        }

        if (group.category == SensorCategory::Temperature)
        {
            std::stable_sort(visibleSensors.begin(), visibleSensors.end(), [](const SensorReading* left, const SensorReading* right)
            {
                return temperatureSensorLessThan(*left, *right);
            });
        }

        if (visibleSensors.isEmpty())
            continue;

        const QString currentGroupKey = groupKey(group.category, group.title);
        usedGroupKeys.insert(currentGroupKey);
        GroupNodeState& groupState = ensureGroup(currentGroupKey, group.title, visibleSensors.size(), visibleGroupIndex);
        const QString groupText = QStringLiteral("%1 (%2)").arg(group.title).arg(visibleSensors.size());
        if (groupState.item->text(0) != groupText)
            groupState.item->setText(0, groupText);

        QSet<QString> usedSensorIds;
        int visibleSensorIndex = 0;
        for (const SensorReading* sensor : visibleSensors)
        {
            usedSensorIds.insert(sensor->sensorId);
            SensorNodeState& sensorState = ensureSensor(groupState, *sensor, visibleSensorIndex);
            updateSensor(sensorState, *sensor);
            ++visibleSensorIndex;
        }

        removeStaleSensors(groupState, usedSensorIds);
        ++visibleGroupIndex;
    }

    removeStaleGroups(usedGroupKeys);
}

CpuSensorTree::GroupNodeState& CpuSensorTree::ensureGroup(const QString& key, const QString& title, int sensorCount, int index)
{
    auto groupIterator = m_groups.find(key);
    if (groupIterator == m_groups.end())
    {
        auto groupItem = std::make_unique<QTreeWidgetItem>(QStringList{QStringLiteral("%1 (%2)").arg(title).arg(sensorCount)});
        groupItem->setFirstColumnSpanned(true);

        QFont groupFont = font();
        groupFont.setWeight(QFont::DemiBold);
        groupItem->setFont(0, groupFont);
        groupItem->setForeground(0, QColor(theme::kAccentCyan));

        QTreeWidgetItem* createdItem = groupItem.get();
        insertTopLevelItem(index, createdItem);
        groupItem.release();

        GroupNodeState state;
        state.item = createdItem;
        groupIterator = m_groups.insert(key, state);
        createdItem->setExpanded(true);
        return *groupIterator;
    }

    GroupNodeState& state = *groupIterator;
    const int currentIndex = indexOfTopLevelItem(state.item);
    if (currentIndex != index)
    {
        std::unique_ptr<QTreeWidgetItem> movedItem(takeTopLevelItem(currentIndex));
        insertTopLevelItem(index, movedItem.get());
        movedItem.release();
    }

    return state;
}

CpuSensorTree::SensorNodeState& CpuSensorTree::ensureSensor(GroupNodeState& group, const SensorReading& sensor, int index)
{
    auto sensorIterator = group.sensors.find(sensor.sensorId);
    if (sensorIterator == group.sensors.end())
    {
        auto sensorItem = std::make_unique<QTreeWidgetItem>(QStringList{sensor.name, QStringLiteral("--")});
        sensorItem->setForeground(0, QColor(theme::kTextPrimary));
        sensorItem->setForeground(1, QColor(theme::kOffline));
        sensorItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);

        QTreeWidgetItem* createdItem = sensorItem.get();
        group.item->insertChild(index, createdItem);
        sensorItem.release();

        SensorNodeState state;
        state.item = createdItem;
        state.valueText = QStringLiteral("--");
        return *group.sensors.insert(sensor.sensorId, state);
    }

    SensorNodeState& state = *sensorIterator;
    const int currentIndex = group.item->indexOfChild(state.item);
    if (currentIndex != index)
    {
        std::unique_ptr<QTreeWidgetItem> movedItem(group.item->takeChild(currentIndex));
        group.item->insertChild(index, movedItem.get());
        movedItem.release();
    }

    return state;
}

void CpuSensorTree::updateSensor(SensorNodeState& state, const SensorReading& sensor)
{
    if (state.item->text(0) != sensor.name)
        state.item->setText(0, sensor.name);

    const QString currentValueText = sensorValueText(sensor);
    if (state.valueText != currentValueText)
    {
        state.item->setText(1, currentValueText);
        state.valueText = currentValueText;
    }

    const bool available = sensor.current.isAvailable();
    if (state.available != available)
    {
        state.item->setForeground(1, available ? QColor(theme::kTextSecondary) : QColor(theme::kOffline));
        state.available = available;
    }
}

void CpuSensorTree::removeStaleSensors(GroupNodeState& group, const QSet<QString>& usedSensorIds)
{
    const auto sensorIds = group.sensors.keys();
    for (const QString& sensorId : sensorIds)
    {
        if (usedSensorIds.contains(sensorId))
            continue;

        const SensorNodeState removedState = group.sensors.take(sensorId);
        const int childIndex = group.item->indexOfChild(removedState.item);
        if (childIndex >= 0)
        {
            std::unique_ptr<QTreeWidgetItem> removedItem(group.item->takeChild(childIndex));
        }
    }
}

void CpuSensorTree::removeStaleGroups(const QSet<QString>& usedGroupKeys)
{
    const auto groupKeys = m_groups.keys();
    for (const QString& key : groupKeys)
    {
        if (usedGroupKeys.contains(key))
            continue;

        const GroupNodeState removedState = m_groups.take(key);
        const int groupIndex = indexOfTopLevelItem(removedState.item);
        if (groupIndex >= 0)
        {
            std::unique_ptr<QTreeWidgetItem> removedItem(takeTopLevelItem(groupIndex));
        }
    }
}

void CpuSensorTree::scrollContentsBy(int dx, int dy)
{
    Q_UNUSED(dx)
    QTreeWidget::scrollContentsBy(0, dy);
}

} // namespace monitor
