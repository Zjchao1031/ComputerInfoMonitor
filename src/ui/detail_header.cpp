#include "ui/detail_header.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStringLiteral>
#include <QToolButton>

namespace monitor
{

DetailHeader::DetailHeader(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(36);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* backButton = new QToolButton(this);
    backButton->setObjectName(QStringLiteral("backButton"));
    backButton->setText(QStringLiteral("← 返回"));
    backButton->setAutoRaise(true);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

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

