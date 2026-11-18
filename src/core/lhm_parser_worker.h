#pragma once

#include "model/monitor_snapshot.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtGlobal>

namespace monitor
{

class LhmParserWorker final : public QObject
{
    Q_OBJECT

public:
    explicit LhmParserWorker(QObject* parent = nullptr);

public slots:
    void parse(quint64 requestSerial, const QByteArray& payload);

signals:
    void parseSucceeded(quint64 requestSerial, const monitor::MonitorSnapshot& snapshot);
    void parseFailed(quint64 requestSerial, const QString& errorMessage);
};

} // namespace monitor
