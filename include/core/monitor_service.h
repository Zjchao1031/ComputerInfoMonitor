#pragma once

#include "core/app_config.h"
#include "model/monitor_snapshot.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QThread>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace monitor
{

class LhmParserWorker;

class MonitorService final : public QObject
{
    Q_OBJECT

public:
    explicit MonitorService(AppConfig config, QObject* parent = nullptr);
    ~MonitorService() override;

    void start();
    void stop();

signals:
    void snapshotUpdated(const monitor::MonitorSnapshot& snapshot);
    void connectionStateChanged(monitor::ConnectionState state);
    void historyUpdated(const QVector<monitor::HistorySeries>& histories);
    void parseRequested(quint64 requestSerial, const QByteArray& payload);

private:
    struct HistoryBuffer
    {
        void configure(QString identifier, QString unit, int capacity);
        void append(const HistoryPoint& point);
        [[nodiscard]] QVector<HistoryPoint> points() const;

        QString identifier;
        QString unit;
        int capacity{0};
        int firstIndex{0};
        int count{0};
        QVector<HistoryPoint> storage;
    };

    void startRequest();
    void handleReplyMetaDataChanged();
    void handleReplyFinished();
    void handleConnectionTimeout();
    void handleReadTimeout();
    void finishWithFailure(ConnectionState failureState, bool abortRequest);
    void handleParseSucceeded(quint64 requestSerial, const MonitorSnapshot& snapshot);
    void handleParseFailed(quint64 requestSerial, const QString& errorMessage);
    void releaseNetworkReply(bool abortRequest);
    [[nodiscard]] bool isActiveRequest(quint64 requestSerial) const;
    void scheduleNextRequest();
    void initializeHistoryBuffers();
    void appendHistory(const MonitorSnapshot& snapshot);
    [[nodiscard]] QVector<HistorySeries> historySeries() const;
    [[nodiscard]] int currentDataAgeMs() const;
    void updateConnectionState();

    AppConfig m_config;
    QThread m_parserThread;
    LhmParserWorker* m_parserWorker{nullptr};
    QNetworkAccessManager* m_networkManager{nullptr};
    QPointer<QNetworkReply> m_reply;
    QTimer* m_refreshTimer{nullptr};
    QTimer* m_connectionTimeoutTimer{nullptr};
    QTimer* m_readTimeoutTimer{nullptr};
    QTimer* m_statusTimer{nullptr};
    QElapsedTimer m_lastSuccessAgeTimer;
    QVector<HistoryBuffer> m_historyBuffers;
    MonitorSnapshot m_latestSnapshot;
    ConnectionState m_lastFailureState{ConnectionState::Starting};
    quint64 m_requestSerial{0};
    quint64 m_activeRequestSerial{0};
    int m_consecutiveFailures{0};
    bool m_started{false};
    bool m_requestActive{false};
    bool m_responseHeadersReceived{false};
    bool m_terminalFailureHandled{false};
    bool m_hasSuccessfulSnapshot{false};
};

} // namespace monitor

