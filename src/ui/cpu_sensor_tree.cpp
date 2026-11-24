#include "ui/cpu_sensor_tree.h"

#include "ui/theme.h"

#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QHash>
#include <QHideEvent>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSize>
#include <QStringLiteral>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <utility>

namespace monitor
{

namespace
{

constexpr int kRowHeight = 20;
constexpr int kValueColumnWidth = 76;
constexpr int kHorizontalPadding = 4;
constexpr int kSensorIndent = 14;
constexpr int kDragPixelsPerRow = 10;

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

class CpuSensorModel final : public QAbstractListModel
{
public:
    enum RowRole
    {
        NameRole = Qt::UserRole + 1,
        ValueRole,
        IsGroupRole,
        AvailableRole
    };

private:
    struct Row
    {
        QString key;
        QString name;
        QString valueText;
        bool isGroup{false};
        bool available{false};
    };

public:
    explicit CpuSensorModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
    }

    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override
    {
        const int count = static_cast<int>(m_rows.size());
        if (!index.isValid() || index.row() < 0 || index.row() >= count)
            return {};

        const Row& row = m_rows.at(index.row());
        switch (role)
        {
        case Qt::DisplayRole:
        case NameRole:
            return row.name;
        case ValueRole:
            return row.valueText;
        case IsGroupRole:
            return row.isGroup;
        case AvailableRole:
            return row.available;
        default:
            return {};
        }
    }

    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        return Qt::ItemIsEnabled;
    }

    [[nodiscard]] bool setGroups(const QVector<SensorGroup>& groups)
    {
        QVector<Row> rows;
        rows.reserve(m_rows.size());

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
            rows.append(Row{
                currentGroupKey,
                QStringLiteral("%1 (%2)").arg(group.title).arg(visibleSensors.size()),
                {},
                true,
                true
            });

            for (const SensorReading* sensor : visibleSensors)
            {
                rows.append(Row{
                    currentGroupKey + QChar(0x1f) + sensor->sensorId,
                    sensor->name,
                    sensorValueText(*sensor),
                    false,
                    sensor->current.isAvailable()
                });
            }
        }

        if (!topologyMatches(rows))
        {
            beginResetModel();
            m_rows = std::move(rows);
            endResetModel();
            return true;
        }

        bool changed = false;
        const int rowCount = static_cast<int>(m_rows.size());
        for (int index = 0; index < rowCount; ++index)
        {
            Row& current = m_rows[index];
            const Row& desired = rows.at(index);
            if (current.name == desired.name && current.valueText == desired.valueText && current.available == desired.available)
                continue;

            current.name = desired.name;
            current.valueText = desired.valueText;
            current.available = desired.available;
            changed = true;
        }

        if (changed)
        {
            emit dataChanged(index(0), index(static_cast<int>(m_rows.size()) - 1), {NameRole, ValueRole, AvailableRole});
        }

        return false;
    }

private:
    [[nodiscard]] bool topologyMatches(const QVector<Row>& rows) const noexcept
    {
        if (m_rows.size() != rows.size())
            return false;

        const int rowCount = static_cast<int>(m_rows.size());
        for (int index = 0; index < rowCount; ++index)
        {
            if (m_rows.at(index).key != rows.at(index).key || m_rows.at(index).isGroup != rows.at(index).isGroup)
                return false;
        }

        return true;
    }

    QVector<Row> m_rows;
};

class SensorRowDelegate final : public QStyledItemDelegate
{
public:
    explicit SensorRowDelegate(const QFont& font, QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_rowFont(font)
        , m_groupFont(font)
        , m_nameColor(QColor(theme::kTextPrimary))
        , m_valueColor(QColor(theme::kTextSecondary))
        , m_offlineColor(QColor(theme::kOffline))
        , m_groupColor(QColor(theme::kAccentCyan))
    {
        m_groupFont.setWeight(QFont::DemiBold);
    }

    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)
        return QSize(0, kRowHeight);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        if (painter == nullptr || !index.isValid())
            return;

        if (option.rect.width() <= 0 || option.rect.height() <= 0)
            return;

        const qreal devicePixelRatio = option.widget != nullptr ? option.widget->devicePixelRatioF() : 1.0;
        auto cachedRow = m_rows.find(index.row());
        if (cachedRow == m_rows.end() || cachedRow->logicalWidth != option.rect.width() || cachedRow->devicePixelRatio != devicePixelRatio)
        {
            CachedRow newRow = createRowPixmap(index, option.rect.width(), devicePixelRatio);
            if (cachedRow == m_rows.end())
                cachedRow = m_rows.insert(index.row(), std::move(newRow));
            else
                *cachedRow = std::move(newRow);
        }

        painter->save();
        painter->setClipRect(option.rect);
        painter->drawPixmap(option.rect.topLeft(), cachedRow->pixmap);
        painter->restore();
    }

    void invalidateRows(int firstRow, int lastRow)
    {
        if (firstRow > lastRow)
            std::swap(firstRow, lastRow);

        for (int row = firstRow; row <= lastRow; ++row)
            m_rows.remove(row);
    }

    void clearCache()
    {
        m_rows.clear();
    }

private:
    struct CachedRow
    {
        QPixmap pixmap;
        int logicalWidth{0};
        qreal devicePixelRatio{1.0};
    };

    [[nodiscard]] CachedRow createRowPixmap(const QModelIndex& index, int logicalWidth, qreal devicePixelRatio) const
    {
        const qreal validDevicePixelRatio = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
        const int pixmapWidth = std::max(1, static_cast<int>(std::ceil(static_cast<qreal>(logicalWidth) * validDevicePixelRatio)));
        const int pixmapHeight = std::max(1, static_cast<int>(std::ceil(static_cast<qreal>(kRowHeight) * validDevicePixelRatio)));

        QPixmap pixmap(pixmapWidth, pixmapHeight);
        pixmap.setDevicePixelRatio(validDevicePixelRatio);
        pixmap.fill(Qt::transparent);

        const bool isGroup = index.data(CpuSensorModel::IsGroupRole).toBool();
        const bool available = index.data(CpuSensorModel::AvailableRole).toBool();
        const QString name = index.data(CpuSensorModel::NameRole).toString();
        const QFont& rowFont = isGroup ? m_groupFont : m_rowFont;
        const QColor& nameColor = isGroup ? m_groupColor : m_nameColor;

        const QRectF rowRect(0.0, 0.0, static_cast<qreal>(logicalWidth), static_cast<qreal>(kRowHeight));
        const QRectF contentRect = rowRect.adjusted(kHorizontalPadding, 0, -kHorizontalPadding, 0);
        const int valueWidth = isGroup ? 0 : kValueColumnWidth;
        QRectF nameRect = contentRect;
        if (!isGroup)
            nameRect.adjust(kSensorIndent, 0, -valueWidth, 0);

        QPainter painter(&pixmap);
        painter.setFont(rowFont);
        painter.setPen(nameColor);
        painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, name);

        if (!isGroup)
        {
            const QString valueText = index.data(CpuSensorModel::ValueRole).toString();
            const QRectF valueRect(contentRect.right() - valueWidth + 1.0, contentRect.top(), valueWidth, contentRect.height());
            painter.setPen(available ? m_valueColor : m_offlineColor);
            painter.drawText(valueRect, Qt::AlignRight | Qt::AlignVCenter, valueText);
        }

        return CachedRow{std::move(pixmap), logicalWidth, validDevicePixelRatio};
    }

    QFont m_rowFont;
    QFont m_groupFont;
    QColor m_nameColor;
    QColor m_valueColor;
    QColor m_offlineColor;
    QColor m_groupColor;
    mutable QHash<int, CachedRow> m_rows;
};

CpuSensorTree::CpuSensorTree(QWidget* parent)
    : QListView(parent)
{
    setObjectName(QStringLiteral("sensorTree"));

    QFont rowFont = font();
    rowFont.setPixelSize(12);
    setFont(rowFont);

    QPalette viewPalette = palette();
    viewPalette.setColor(QPalette::Base, Qt::transparent);
    viewPalette.setColor(QPalette::Window, Qt::transparent);
    setPalette(viewPalette);
    viewport()->setPalette(viewPalette);
    viewport()->setAttribute(Qt::WA_TranslucentBackground, true);
    viewport()->setAutoFillBackground(false);

    m_model = new CpuSensorModel(this);
    setModel(m_model);
    m_delegate = new SensorRowDelegate(font(), this);
    setItemDelegate(m_delegate);

    connect(m_model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& topLeft, const QModelIndex& bottomRight)
    {
        if (m_delegate != nullptr && topLeft.isValid() && bottomRight.isValid())
            m_delegate->invalidateRows(topLeft.row(), bottomRight.row());
    });
    connect(m_model, &QAbstractItemModel::modelReset, this, [this]()
    {
        if (m_delegate != nullptr)
            m_delegate->clearCache();
    });

    setViewMode(QListView::ListMode);
    setFlow(QListView::TopToBottom);
    setWrapping(false);
    setMovement(QListView::Static);
    setResizeMode(QListView::Adjust);
    setLayoutMode(QListView::Batched);
    setBatchSize(100);
    setSpacing(0);
    setUniformItemSizes(true);
    setWordWrap(false);
    setTextElideMode(Qt::ElideNone);
    setAlternatingRowColors(false);
    setFocusPolicy(Qt::NoFocus);
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setDragEnabled(false);
    setAcceptDrops(false);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerItem);

    viewport()->installEventFilter(this);
}

void CpuSensorTree::beginScrolling()
{
    if (m_scrolling)
        return;

    m_scrolling = true;
    emit scrollingChanged(true);
}

void CpuSensorTree::endScrolling()
{
    if (!m_scrolling)
        return;

    m_scrolling = false;
    emit scrollingChanged(false);
}

bool CpuSensorTree::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != viewport() || event == nullptr)
        return QListView::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonPress)
    {
        const auto* mouseEvent = static_cast<const QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton)
            return QListView::eventFilter(watched, event);

        m_leftButtonPressed = true;
        m_lastMouseY = mouseEvent->position().y();
        m_pendingDragPixels = 0.0;
        beginScrolling();
        return true;
    }

    if (event->type() == QEvent::MouseMove && m_leftButtonPressed)
    {
        const auto* mouseEvent = static_cast<const QMouseEvent*>(event);
        const qreal currentY = mouseEvent->position().y();
        m_pendingDragPixels += currentY - m_lastMouseY;
        m_lastMouseY = currentY;

        const int rows = static_cast<int>(m_pendingDragPixels / static_cast<qreal>(kDragPixelsPerRow));
        if (rows != 0)
        {
            QScrollBar* scrollBar = verticalScrollBar();
            const int newValue = std::clamp(scrollBar->value() - rows, scrollBar->minimum(), scrollBar->maximum());
            scrollBar->setValue(newValue);
            m_pendingDragPixels -= rows * kDragPixelsPerRow;
        }

        return true;
    }

    if (event->type() == QEvent::MouseButtonRelease && m_leftButtonPressed)
    {
        const auto* mouseEvent = static_cast<const QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            m_leftButtonPressed = false;
            m_lastMouseY = 0.0;
            m_pendingDragPixels = 0.0;
            endScrolling();
            return true;
        }
    }

    if (event->type() == QEvent::UngrabMouse)
    {
        m_leftButtonPressed = false;
        m_lastMouseY = 0.0;
        m_pendingDragPixels = 0.0;
        endScrolling();
    }

    return QListView::eventFilter(watched, event);
}

void CpuSensorTree::changeEvent(QEvent* event)
{
    QListView::changeEvent(event);

    if (event == nullptr || m_delegate == nullptr)
        return;

    if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange || event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        m_delegate->clearCache();
}

void CpuSensorTree::hideEvent(QHideEvent* event)
{
    m_leftButtonPressed = false;
    m_lastMouseY = 0.0;
    m_pendingDragPixels = 0.0;
    endScrolling();
    QListView::hideEvent(event);
}

void CpuSensorTree::setGroups(const QVector<SensorGroup>& groups)
{
    QScrollBar* scrollBar = verticalScrollBar();
    const int scrollPosition = scrollBar->value();
    if (!m_model->setGroups(groups))
        return;

    QTimer::singleShot(0, this, [scrollBar, scrollPosition]()
    {
        scrollBar->setValue(qMin(scrollPosition, scrollBar->maximum()));
    });
}

} // namespace monitor
