#pragma once

#include <QString>
#include <QtGlobal>

namespace monitor
{

struct AppConfig
{
    QString pcIpAddress;
    quint16 lhmPort{8085};
    int historyWindowSeconds{60};
    int refreshPeriodMs{1000};
};

} // namespace monitor

