#pragma once

#include "model/monitor_snapshot.h"

#include <QWidget>

class QLabel;

namespace monitor
{

class ConnectionStatusWidget final : public QWidget
{
public:
    explicit ConnectionStatusWidget(QWidget* parent = nullptr);

    void setStatus(ConnectionState state);

private:
    QLabel* m_dotLabel{nullptr};
    QLabel* m_stateLabel{nullptr};
    ConnectionState m_state{ConnectionState::Starting};
    bool m_hasState{false};
};

} // namespace monitor

