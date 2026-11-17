#include "ui/cpu_sensor_tree.h"

#include "ui/theme.h"

#include <QColor>
#include <QFont>
#include <QHash>
#include <QHeaderView>
#include <QStringList>
#include <QTreeWidgetItem>

#include <cmath>

namespace monitor
{

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
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setExpandsOnDoubleClick(true);

    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

void CpuSensorTree::setGroups(const QVector<SensorGroup>& groups)
{
    QHash<QString, bool> expansionState;
    for (int index = 0; index < topLevelItemCount(); ++index)
    {
        QTreeWidgetItem* item = topLevelItem(index);
        expansionState.insert(item->text(0), item->isExpanded());
    }

    clear();

    for (const SensorGroup& group : groups)
    {
        if (group.sensors.isEmpty())
            continue;

        auto* groupItem = new QTreeWidgetItem(this, QStringList{QStringLiteral("%1 (%2)").arg(group.title).arg(group.sensors.size())});
        groupItem->setFirstColumnSpanned(true);

        QFont groupFont = font();
        groupFont.setWeight(QFont::DemiBold);
        groupItem->setFont(0, groupFont);
        groupItem->setForeground(0, QColor(theme::kAccentCyan));

        for (const SensorReading& sensor : group.sensors)
        {
            QString valueText = QStringLiteral("--");
            if (sensor.current.value.has_value() && std::isfinite(*sensor.current.value))
                valueText = QStringLiteral("%1 %2").arg(QString::number(*sensor.current.value, 'f', 1)).arg(sensor.current.unit);

            auto* sensorItem = new QTreeWidgetItem(groupItem, QStringList{sensor.name, valueText});
            sensorItem->setForeground(0, QColor(theme::kTextPrimary));
            sensorItem->setForeground(1, sensor.current.isAvailable() ? QColor(theme::kTextSecondary) : QColor(theme::kOffline));
        }

        groupItem->setExpanded(expansionState.value(group.title, true));
    }
}

} // namespace monitor
