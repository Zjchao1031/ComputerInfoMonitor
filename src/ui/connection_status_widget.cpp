#include "ui/connection_status_widget.h"

#include "ui/theme.h"

#include <QColor>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringLiteral>

namespace monitor
{

ConnectionStatusWidget::ConnectionStatusWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_dotLabel = new QLabel(this);
    m_dotLabel->setFixedSize(12, 12);

    m_stateLabel = new QLabel(this);
    m_stateLabel->setObjectName(QStringLiteral("connectionText"));

    m_lastUpdatedLabel = new QLabel(this);
    m_lastUpdatedLabel->setObjectName(QStringLiteral("lastUpdated"));
    m_lastUpdatedLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(m_dotLabel);
    layout->addWidget(m_stateLabel);
    layout->addSpacing(8);
    layout->addWidget(m_lastUpdatedLabel);
}

void ConnectionStatusWidget::setStatus(ConnectionState state, const QDateTime& lastUpdated)
{
    const QColor stateColor = theme::connectionStateColor(state);
    m_dotLabel->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 6px;").arg(stateColor.name()));
    m_stateLabel->setText(theme::connectionStateText(state));
    m_stateLabel->setStyleSheet(QStringLiteral("color: %1;").arg(stateColor.name()));

    if (lastUpdated.isValid())
    {
        m_lastUpdatedLabel->setText(QStringLiteral("最后更新 %1").arg(lastUpdated.toString(QStringLiteral("HH:mm:ss"))));
    }
    else
    {
        m_lastUpdatedLabel->setText(QStringLiteral("暂无更新时间"));
    }
}

} // namespace monitor

