#include "ui/connection_status_widget.h"

#include "ui/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QStringLiteral>

namespace monitor
{

namespace
{

QString connectionTone(ConnectionState state)
{
    switch (state)
    {
    case ConnectionState::Starting:
        return QStringLiteral("starting");
    case ConnectionState::Online:
        return QStringLiteral("online");
    case ConnectionState::Delayed:
    case ConnectionState::ParseError:
        return QStringLiteral("warning");
    case ConnectionState::Offline:
    case ConnectionState::NetworkUnavailable:
    case ConnectionState::ServiceUnavailable:
        return QStringLiteral("critical");
    }

    return QStringLiteral("unavailable");
}

void setConnectionTone(QWidget* widget, const QString& tone)
{
    if (widget->property("connectionTone").toString() == tone)
        return;

    widget->setProperty("connectionTone", tone);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

} // namespace

ConnectionStatusWidget::ConnectionStatusWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_dotLabel = new QLabel(this);
    m_dotLabel->setObjectName(QStringLiteral("connectionDot"));
    m_dotLabel->setFixedSize(12, 12);

    m_stateLabel = new QLabel(this);
    m_stateLabel->setObjectName(QStringLiteral("connectionText"));

    layout->addWidget(m_dotLabel);
    layout->addWidget(m_stateLabel);
}

void ConnectionStatusWidget::setStatus(ConnectionState state)
{
    if (!m_hasState || state != m_state)
    {
        m_state = state;
        m_hasState = true;

        const QString tone = connectionTone(state);
        m_stateLabel->setText(theme::connectionStateText(state));
        setConnectionTone(m_dotLabel, tone);
    }
}

} // namespace monitor

