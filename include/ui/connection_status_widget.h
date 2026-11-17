#pragma once

#include "model/monitor_snapshot.h"

#include <QWidget>

class QDateTime;
class QLabel;

namespace monitor
{

class ConnectionStatusWidget final : public QWidget
{
public:
    explicit ConnectionStatusWidget(QWidget* parent = nullptr);

    void setStatus(ConnectionState state, const QDateTime& lastUpdated);

private:
    QLabel* m_dotLabel{nullptr};
    QLabel* m_stateLabel{nullptr};
    QLabel* m_lastUpdatedLabel{nullptr};
};

} // namespace monitor

