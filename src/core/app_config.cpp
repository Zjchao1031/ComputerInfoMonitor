#include "core/app_config.h"

#include "core/fixed_parameters.h"

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QSettings>
#include <QStringLiteral>

#include <optional>
#include <utility>

namespace monitor
{

namespace
{

QString configError(const QString& filePath, const QString& message)
{
    return QStringLiteral("配置文件 \"%1\"：%2").arg(filePath, message);
}

QString requiredSetting(const QSettings& settings, const QString& key, const QString& displayName, const QString& filePath, QString& errorMessage)
{
    if (!settings.contains(key))
    {
        errorMessage = configError(filePath, QStringLiteral("缺少必填项 %1。").arg(displayName));
        return {};
    }

    const QString value = settings.value(key).toString().trimmed();
    if (value.isEmpty())
    {
        errorMessage = configError(filePath, QStringLiteral("必填项 %1 为空。").arg(displayName));
    }

    return value;
}

bool isValidIpv4Address(const QString& value)
{
    QHostAddress address;
    return address.setAddress(value) && address.protocol() == QAbstractSocket::IPv4Protocol;
}

QString quotedValue(const QString& value)
{
    return QStringLiteral("\"%1\"").arg(value);
}

} // namespace

QString AppConfig::defaultFilePath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("monitor.ini"));
}

std::optional<AppConfig> AppConfig::loadFromFile(const QString& filePath, QString& errorMessage)
{
    errorMessage.clear();

    if (filePath.trimmed().isEmpty())
    {
        errorMessage = QStringLiteral("配置文件路径为空。");
        return std::nullopt;
    }

    const QFileInfo configFile(filePath);
    const QString absoluteFilePath = configFile.absoluteFilePath();
    if (!configFile.exists())
    {
        errorMessage = QStringLiteral("配置文件 \"%1\" 不存在。").arg(absoluteFilePath);
        return std::nullopt;
    }

    if (!configFile.isFile())
    {
        errorMessage = QStringLiteral("配置文件 \"%1\" 不是普通文件。").arg(absoluteFilePath);
        return std::nullopt;
    }

    QSettings settings(absoluteFilePath, QSettings::IniFormat);
    settings.setFallbacksEnabled(false);
    settings.sync();

    if (settings.status() == QSettings::FormatError)
    {
        errorMessage = QStringLiteral("配置文件 \"%1\" 格式错误，无法按 UTF-8 INI 格式解析。").arg(absoluteFilePath);
        return std::nullopt;
    }

    if (settings.status() != QSettings::NoError)
    {
        errorMessage = QStringLiteral("配置文件 \"%1\" 无法读取，请检查文件权限和编码。").arg(absoluteFilePath);
        return std::nullopt;
    }

    const QString pcIpAddress = requiredSetting(settings, QStringLiteral("server/pc_ip"), QStringLiteral("[server] pc_ip"), absoluteFilePath, errorMessage);
    if (pcIpAddress.isEmpty())
        return std::nullopt;

    if (!isValidIpv4Address(pcIpAddress))
    {
        errorMessage = configError(absoluteFilePath, QStringLiteral("[server] pc_ip 必须是有效的 IPv4 地址，当前值为 %1。").arg(quotedValue(pcIpAddress)));
        return std::nullopt;
    }

    const QString portText = requiredSetting(settings, QStringLiteral("server/port"), QStringLiteral("[server] port"), absoluteFilePath, errorMessage);
    if (portText.isEmpty())
        return std::nullopt;

    bool portIsValid = false;
    const uint portValue = portText.toUInt(&portIsValid, 10);
    if (!portIsValid || portValue == 0 || portValue > 65535)
    {
        errorMessage = configError(absoluteFilePath, QStringLiteral("[server] port 必须是 1～65535 范围内的整数，当前值为 %1。").arg(quotedValue(portText)));
        return std::nullopt;
    }

    const QString historyWindowText = requiredSetting(settings, QStringLiteral("monitor/history_window_seconds"), QStringLiteral("[monitor] history_window_seconds"), absoluteFilePath, errorMessage);
    if (historyWindowText.isEmpty())
        return std::nullopt;

    bool historyWindowIsValid = false;
    const int historyWindowSeconds = historyWindowText.toInt(&historyWindowIsValid, 10);
    if (!historyWindowIsValid || historyWindowSeconds <= 0)
    {
        errorMessage = configError(absoluteFilePath, QStringLiteral("[monitor] history_window_seconds 必须是正整数，当前值为 %1。").arg(quotedValue(historyWindowText)));
        return std::nullopt;
    }

    const QString refreshPeriodText = requiredSetting(settings, QStringLiteral("monitor/refresh_period_ms"), QStringLiteral("[monitor] refresh_period_ms"), absoluteFilePath, errorMessage);
    if (refreshPeriodText.isEmpty())
        return std::nullopt;

    bool refreshPeriodIsValid = false;
    const int refreshPeriodMs = refreshPeriodText.toInt(&refreshPeriodIsValid, 10);
    if (!refreshPeriodIsValid || refreshPeriodMs < 250)
    {
        errorMessage = configError(absoluteFilePath, QStringLiteral("[monitor] refresh_period_ms 必须是不小于 250 的整数，当前值为 %1。").arg(quotedValue(refreshPeriodText)));
        return std::nullopt;
    }

    const qint64 requestedSampleCount = (static_cast<qint64>(historyWindowSeconds) * 1000 + refreshPeriodMs - 1) / refreshPeriodMs;
    if (requestedSampleCount > fixed::kMaximumHistorySamples)
    {
        errorMessage = configError(absoluteFilePath, QStringLiteral("按 history_window_seconds=%1 和 refresh_period_ms=%2 计算需要 %3 个历史采样点，超过固定上限 %4。").arg(historyWindowSeconds).arg(refreshPeriodMs).arg(requestedSampleCount).arg(fixed::kMaximumHistorySamples));
        return std::nullopt;
    }

    return AppConfig(pcIpAddress, static_cast<quint16>(portValue), historyWindowSeconds, refreshPeriodMs, static_cast<int>(requestedSampleCount));
}

AppConfig::AppConfig(QString pcIpAddress, quint16 lhmPort, int historyWindowSeconds, int refreshPeriodMs, int historySampleCapacity)
    : m_pcIpAddress(std::move(pcIpAddress))
    , m_lhmPort(lhmPort)
    , m_historyWindowSeconds(historyWindowSeconds)
    , m_refreshPeriodMs(refreshPeriodMs)
    , m_historySampleCapacity(historySampleCapacity)
{
}

const QString& AppConfig::pcIpAddress() const noexcept
{
    return m_pcIpAddress;
}

quint16 AppConfig::lhmPort() const noexcept
{
    return m_lhmPort;
}

int AppConfig::historyWindowSeconds() const noexcept
{
    return m_historyWindowSeconds;
}

int AppConfig::refreshPeriodMs() const noexcept
{
    return m_refreshPeriodMs;
}

int AppConfig::historySampleCapacity() const noexcept
{
    return m_historySampleCapacity;
}

} // namespace monitor
