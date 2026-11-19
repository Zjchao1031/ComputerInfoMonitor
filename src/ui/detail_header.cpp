#include "ui/detail_header.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPointF>
#include <QStringLiteral>
#include <QToolButton>
#include <QTouchEvent>

namespace monitor
{

namespace
{

// linuxfb 触摸输入不保证生成 QToolButton 所需的完整鼠标按压/释放序列。
class TouchToolButton final : public QToolButton
{
public:
    explicit TouchToolButton(QWidget* parent)
        : QToolButton(parent)
    {
        setAttribute(Qt::WA_AcceptTouchEvents, true);
    }

protected:
    bool event(QEvent* event) override
    {
        switch (event->type())
        {
        case QEvent::TouchBegin:
            return handleTouchBegin(static_cast<QTouchEvent*>(event));
        case QEvent::TouchUpdate:
            return handleTouchUpdate(static_cast<QTouchEvent*>(event));
        case QEvent::TouchEnd:
            return handleTouchEnd(static_cast<QTouchEvent*>(event));
        case QEvent::TouchCancel:
            resetTouchState();
            event->accept();
            return true;
        default:
            return QToolButton::event(event);
        }
    }

private:
    bool handleTouchBegin(QTouchEvent* event)
    {
        if (m_touchPointId != -1 || event->points().isEmpty())
        {
            event->accept();
            return true;
        }

        const auto& point = event->points().constFirst();
        m_touchPointId = point.id();
        m_lastTouchPosition = point.position();
        m_touchInside = rect().contains(m_lastTouchPosition.toPoint());
        setDown(m_touchInside);
        event->accept();
        return true;
    }

    bool handleTouchUpdate(QTouchEvent* event)
    {
        for (const auto& point : event->points())
        {
            if (point.id() != m_touchPointId)
                continue;

            m_lastTouchPosition = point.position();
            m_touchInside = rect().contains(m_lastTouchPosition.toPoint());
            setDown(m_touchInside);
            break;
        }

        event->accept();
        return true;
    }

    bool handleTouchEnd(QTouchEvent* event)
    {
        bool releaseInside = m_touchInside;
        for (const auto& point : event->points())
        {
            if (point.id() != m_touchPointId)
                continue;

            m_lastTouchPosition = point.position();
            releaseInside = rect().contains(m_lastTouchPosition.toPoint());
            break;
        }

        const bool shouldClick = m_touchPointId != -1 && m_touchInside && releaseInside;
        resetTouchState();
        if (shouldClick)
            click();

        event->accept();
        return true;
    }

    void resetTouchState()
    {
        m_touchPointId = -1;
        m_touchInside = false;
        setDown(false);
    }

    int m_touchPointId{-1};
    QPointF m_lastTouchPosition;
    bool m_touchInside{false};
};

} // namespace

DetailHeader::DetailHeader(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(36);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* backButton = new TouchToolButton(this);
    backButton->setObjectName(QStringLiteral("backButton"));
    backButton->setText(QStringLiteral("← 返回"));
    backButton->setAutoRaise(true);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    backButton->setMinimumSize(64, 36);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("detailTitle"));
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    layout->addWidget(backButton);
    layout->addWidget(m_titleLabel, 1);

    connect(backButton, &QToolButton::clicked, this, &DetailHeader::backRequested);
}

void DetailHeader::setTitle(const QString& title)
{
    m_titleLabel->setText(title);
}

} // namespace monitor

