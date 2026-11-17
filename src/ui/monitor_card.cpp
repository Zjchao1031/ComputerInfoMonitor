#include "ui/monitor_card.h"

#include <QLabel>
#include <QMouseEvent>
#include <QStringLiteral>
#include <QStyle>
#include <QVBoxLayout>

namespace monitor
{

MonitorCard::MonitorCard(const QString& title, QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("monitorCard"));
    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* cardLayout = new QVBoxLayout(this);
    cardLayout->setContentsMargins(4, 4, 4, 4);
    cardLayout->setSpacing(4);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName(QStringLiteral("cardTitle"));
    cardLayout->addWidget(titleLabel);

    m_hardwareNameLabel = new QLabel(this);
    m_hardwareNameLabel->setObjectName(QStringLiteral("hardwareName"));
    m_hardwareNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    cardLayout->addWidget(m_hardwareNameLabel);

    auto* separator = new QFrame(this);
    separator->setObjectName(QStringLiteral("cardSeparator"));
    separator->setFrameShape(QFrame::NoFrame);
    cardLayout->addWidget(separator);

    m_bodyLayout = new QVBoxLayout();
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(4);
    cardLayout->addLayout(m_bodyLayout, 1);
}

void MonitorCard::setHardwareName(const QString& name)
{
    m_hardwareNameLabel->setText(name);
    m_hardwareNameLabel->setVisible(!name.isEmpty());
}

void MonitorCard::setInteractive(bool interactive)
{
    setProperty("interactive", interactive);
    setFocusPolicy(Qt::NoFocus);
    setCursor(interactive ? Qt::PointingHandCursor : Qt::ArrowCursor);
    setAttribute(Qt::WA_Hover, interactive);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

QVBoxLayout* MonitorCard::bodyLayout() const noexcept
{
    return m_bodyLayout;
}

void MonitorCard::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()))
        emit activated();

    QFrame::mouseReleaseEvent(event);
}

} // namespace monitor

