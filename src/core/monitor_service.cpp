#include "core/monitor_service.h"

#include "core/fixed_parameters.h"
#include "lhm_parser_worker.h"
#include "model/history_series_ids.h"

#include <QDateTime>
#include <QMetaType>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QVariant>

#include <algorithm>
#include <iterator>
#include <limits>
#include <utility>

namespace monitor
{

namespace
{

constexpr int kConnectionStateRefreshPeriodMs = 250;

ConnectionState connectionStateForNetworkError(QNetworkReply::NetworkError error)
{
    switch (error)
    {
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::UnknownNetworkError:
        return ConnectionState::NetworkUnavailable;
    case QNetworkReply::NoError:
        return ConnectionState::Online;
    default:
        return ConnectionState::ServiceUnavailable;
    }
}

bool responseLengthIsComplete(QNetworkReply* reply, const QByteArray& payload)
{
    const QVariant contentLengthValue = reply->header(QNetworkRequest::ContentLengthHeader);
    if (!contentLengthValue.isValid())
        return true;

    bool conversionSucceeded = false;
    const qint64 contentLength = contentLengthValue.toLongLong(&conversionSucceeded);
    if (!conversionSucceeded || contentLength < 0)
        return true;

    return payload.size() == contentLength;
}

} // namespace

void MonitorService::HistoryBuffer::configure(QString newIdentifier, QString newUnit, int newCapacity)
{
    identifier = std::move(newIdentifier);
    unit = std::move(newUnit);
    capacity = std::max(0, newCapacity);
    firstIndex = 0;
    count = 0;
    storage.clear();
    storage.resize(capacity);
}

void MonitorService::HistoryBuffer::append(const HistoryPoint& point)
{
    if (capacity <= 0)
        return;

    if (count < capacity)
    {
        const int index = (firstIndex + count) % capacity;
        storage[index] = point;
        ++count;
        return;
    }

    storage[firstIndex] = point;
    firstIndex = (firstIndex + 1) % capacity;
}

QVector<HistoryPoint> MonitorService::HistoryBuffer::points() const
{
    QVector<HistoryPoint> orderedPoints;
    orderedPoints.reserve(count);
    for (int offset = 0; offset < count; ++offset)
    {
        const int index = (firstIndex + offset) % capacity;
        orderedPoints.append(storage.at(index));
    }

    return orderedPoints;
}

MonitorService::MonitorService(AppConfig config, QObject* parent)
    : QObject(parent)
    , m_config(std::move(config))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_refreshTimer(new QTimer(this))
    , m_connectionTimeoutTimer(new QTimer(this))
    , m_readTimeoutTimer(new QTimer(this))
    , m_statusTimer(new QTimer(this))
{
    m_refreshTimer->setSingleShot(true);
    m_connectionTimeoutTimer->setSingleShot(true);
    m_readTimeoutTimer->setSingleShot(true);
    m_statusTimer->setInterval(kConnectionStateRefreshPeriodMs);

    connect(m_refreshTimer, &QTimer::timeout, this, &MonitorService::startRequest);
    connect(m_connectionTimeoutTimer, &QTimer::timeout, this, &MonitorService::handleConnectionTimeout);
    connect(m_readTimeoutTimer, &QTimer::timeout, this, &MonitorService::handleReadTimeout);
    connect(m_statusTimer, &QTimer::timeout, this, &MonitorService::updateConnectionState);

    qRegisterMetaType<MonitorSnapshot>("monitor::MonitorSnapshot");
    m_parserThread.setObjectName(QStringLiteral("LhmParserThread"));
    m_parserWorker = new LhmParserWorker;
    m_parserWorker->moveToThread(&m_parserThread);
    connect(&m_parserThread, &QThread::finished, m_parserWorker, &QObject::deleteLater);
    connect(this, &MonitorService::parseRequested, m_parserWorker, &LhmParserWorker::parse, Qt::QueuedConnection);
    connect(m_parserWorker, &LhmParserWorker::parseSucceeded, this, &MonitorService::handleParseSucceeded, Qt::QueuedConnection);
    connect(m_parserWorker, &LhmParserWorker::parseFailed, this, &MonitorService::handleParseFailed, Qt::QueuedConnection);
    m_parserThread.start();

    initializeHistoryBuffers();
}

MonitorService::~MonitorService()
{
    stop();
    m_parserThread.quit();
    m_parserThread.wait();
    m_parserWorker = nullptr;
}

void MonitorService::stop()
{
    m_started = false;
    m_refreshTimer->stop();
    m_connectionTimeoutTimer->stop();
    m_readTimeoutTimer->stop();
    m_statusTimer->stop();

    releaseNetworkReply(true);
    m_requestActive = false;
}

void MonitorService::start()
{
    if (m_started)
        return;

    m_started = true;
    m_statusTimer->start();
    startRequest();
}

void MonitorService::startRequest()
{
    if (!m_started || m_requestActive)
        return;

    m_requestActive = true;
    m_responseHeadersReceived = false;
    m_terminalFailureHandled = false;
    m_activeRequestSerial = ++m_requestSerial;

    QUrl url;
    url.setScheme(QStringLiteral("http"));
    url.setHost(m_config.pcIpAddress());
    url.setPort(static_cast<int>(m_config.lhmPort()));
    url.setPath(QString::fromLatin1(fixed::kDataPath));

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QVariant::fromValue(QNetworkRequest::ManualRedirectPolicy));
    request.setRawHeader(QByteArrayLiteral("Accept-Encoding"), QByteArrayLiteral("identity"));

    m_reply = m_networkManager->get(request);
    connect(m_reply.data(), &QNetworkReply::metaDataChanged, this, &MonitorService::handleReplyMetaDataChanged);
    connect(m_reply.data(), &QNetworkReply::finished, this, &MonitorService::handleReplyFinished);
    m_connectionTimeoutTimer->start(fixed::kConnectionTimeoutMs);
}

void MonitorService::handleReplyMetaDataChanged()
{
    if (!m_requestActive || m_responseHeadersReceived || m_reply.isNull() || m_terminalFailureHandled)
        return;

    const QVariant statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (!statusCode.isValid())
        return;

    m_responseHeadersReceived = true;
    m_connectionTimeoutTimer->stop();
    m_readTimeoutTimer->start(fixed::kReadTimeoutMs);
}

void MonitorService::handleReplyFinished()
{
    if (!m_requestActive || m_reply.isNull() || m_terminalFailureHandled)
        return;

    QNetworkReply* reply = m_reply.data();
    m_connectionTimeoutTimer->stop();
    m_readTimeoutTimer->stop();

    if (reply->error() != QNetworkReply::NoError)
    {
        finishWithFailure(connectionStateForNetworkError(reply->error()), false);
        return;
    }

    const QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (!statusCode.isValid() || statusCode.toInt() != 200)
    {
        finishWithFailure(ConnectionState::ServiceUnavailable, false);
        return;
    }

    const QByteArray payload = reply->readAll();
    if (!responseLengthIsComplete(reply, payload))
    {
        finishWithFailure(ConnectionState::ParseError, false);
        return;
    }

    const quint64 requestSerial = m_activeRequestSerial;
    releaseNetworkReply(false);
    emit parseRequested(requestSerial, payload);
}

void MonitorService::handleConnectionTimeout()
{
    if (m_requestActive && !m_responseHeadersReceived)
        finishWithFailure(ConnectionState::ServiceUnavailable, true);
}

void MonitorService::handleReadTimeout()
{
    if (m_requestActive)
        finishWithFailure(ConnectionState::ServiceUnavailable, true);
}

void MonitorService::finishWithFailure(ConnectionState failureState, bool abortRequest)
{
    if (!m_requestActive || m_terminalFailureHandled)
        return;

    m_terminalFailureHandled = true;
    releaseNetworkReply(abortRequest);
    m_requestActive = false;
    ++m_consecutiveFailures;
    m_lastFailureState = failureState;
    updateConnectionState();
    scheduleNextRequest();
}

void MonitorService::handleParseSucceeded(quint64 requestSerial, const MonitorSnapshot& snapshot)
{
    if (!isActiveRequest(requestSerial))
        return;

    const ConnectionState previousState = m_latestSnapshot.connectionState;
    m_latestSnapshot = snapshot;
    m_latestSnapshot.connectionState = ConnectionState::Online;
    m_latestSnapshot.receivedAt = QDateTime::currentDateTime();
    m_consecutiveFailures = 0;
    m_lastFailureState = ConnectionState::Online;
    m_hasSuccessfulSnapshot = true;
    m_lastSuccessAgeTimer.start();
    appendHistory(m_latestSnapshot);

    m_requestActive = false;
    if (previousState != ConnectionState::Online)
        emit connectionStateChanged(ConnectionState::Online);

    emit snapshotUpdated(m_latestSnapshot);
    emit historyUpdated(historySeries());
    scheduleNextRequest();
}

void MonitorService::handleParseFailed(quint64 requestSerial, const QString& errorMessage)
{
    if (!isActiveRequest(requestSerial))
        return;

    Q_UNUSED(errorMessage)
    finishWithFailure(ConnectionState::ParseError, false);
}

void MonitorService::releaseNetworkReply(bool abortRequest)
{
    m_connectionTimeoutTimer->stop();
    m_readTimeoutTimer->stop();

    if (!m_reply.isNull())
    {
        QNetworkReply* reply = m_reply.data();
        m_reply.clear();
        QObject::disconnect(reply, nullptr, this, nullptr);
        if (abortRequest)
            reply->abort();
        reply->deleteLater();
    }
}

bool MonitorService::isActiveRequest(quint64 requestSerial) const
{
    return m_started && m_requestActive && requestSerial == m_activeRequestSerial;
}

void MonitorService::scheduleNextRequest()
{
    if (m_started)
        m_refreshTimer->start(m_config.refreshPeriodMs());
}

void MonitorService::initializeHistoryBuffers()
{
    struct HistoryDefinition
    {
        const char* identifier;
        const char* unit;
    };

    static constexpr HistoryDefinition definitions[] = {
        {history_id::kCpuPower, "W"},
        {history_id::kCpuClock, "MHz"},
        {history_id::kCpuTemperature, "°C"},
        {history_id::kGpuClock, "MHz"},
        {history_id::kGpuTemperature, "°C"},
        {history_id::kGpuMemoryUsed, "MB"},
        {history_id::kMemoryUsed, "GB"}
    };

    m_historyBuffers.clear();
    m_historyBuffers.reserve(static_cast<int>(std::size(definitions)));
    for (const HistoryDefinition& definition : definitions)
    {
        HistoryBuffer buffer;
        buffer.configure(QString::fromLatin1(definition.identifier), QString::fromUtf8(definition.unit), m_config.historySampleCapacity());
        m_historyBuffers.append(std::move(buffer));
    }
}

void MonitorService::appendHistory(const MonitorSnapshot& snapshot)
{
    if (m_historyBuffers.size() != 7)
        return;

    const QDateTime sampleTime = snapshot.receivedAt;
    m_historyBuffers[0].append(HistoryPoint{sampleTime, snapshot.cpu.packagePower.value});
    m_historyBuffers[1].append(HistoryPoint{sampleTime, snapshot.cpu.maximumPCoreClock.value});
    m_historyBuffers[2].append(HistoryPoint{sampleTime, snapshot.cpu.coreMaximumTemperature.value});
    m_historyBuffers[3].append(HistoryPoint{sampleTime, snapshot.gpu.coreClock.value});
    m_historyBuffers[4].append(HistoryPoint{sampleTime, snapshot.gpu.coreTemperature.value});
    m_historyBuffers[5].append(HistoryPoint{sampleTime, snapshot.gpu.memoryUsed.value});
    m_historyBuffers[6].append(HistoryPoint{sampleTime, snapshot.memory.used.value});
}

QVector<HistorySeries> MonitorService::historySeries() const
{
    QVector<HistorySeries> histories;
    histories.reserve(m_historyBuffers.size());
    for (const HistoryBuffer& buffer : m_historyBuffers)
    {
        HistorySeries history;
        history.identifier = buffer.identifier;
        history.unit = buffer.unit;
        history.windowSeconds = m_config.historyWindowSeconds();
        history.sampleIntervalMs = m_config.refreshPeriodMs();
        history.points = buffer.points();
        histories.append(std::move(history));
    }

    return histories;
}

int MonitorService::currentDataAgeMs() const
{
    if (!m_hasSuccessfulSnapshot || !m_lastSuccessAgeTimer.isValid())
        return -1;

    const qint64 age = m_lastSuccessAgeTimer.elapsed();
    if (age > static_cast<qint64>(std::numeric_limits<int>::max()))
        return std::numeric_limits<int>::max();

    return static_cast<int>(age);
}

void MonitorService::updateConnectionState()
{
    if (!m_started)
        return;

    const int dataAgeMs = currentDataAgeMs();
    ConnectionState desiredState = ConnectionState::Starting;

    if (m_hasSuccessfulSnapshot)
    {
        if (m_consecutiveFailures >= 5 || dataAgeMs >= fixed::kOfflineDataThresholdMs)
            desiredState = ConnectionState::Offline;
        else if (m_consecutiveFailures >= 3 || dataAgeMs >= fixed::kDelayedDataThresholdMs)
            desiredState = ConnectionState::Delayed;
        else if (m_consecutiveFailures > 0)
            desiredState = m_lastFailureState;
        else
            desiredState = ConnectionState::Online;
    }
    else if (m_consecutiveFailures >= 5)
    {
        desiredState = ConnectionState::Offline;
    }
    else if (m_consecutiveFailures >= 3)
    {
        desiredState = ConnectionState::Delayed;
    }
    else if (m_consecutiveFailures > 0)
    {
        desiredState = m_lastFailureState;
    }

    if (desiredState == m_latestSnapshot.connectionState)
        return;

    m_latestSnapshot.connectionState = desiredState;
    emit connectionStateChanged(desiredState);
    emit snapshotUpdated(m_latestSnapshot);
}

} // namespace monitor

