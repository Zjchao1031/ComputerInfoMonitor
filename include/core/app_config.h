#pragma once

#include <QString>
#include <QtGlobal>

#include <optional>

namespace monitor
{

class AppConfig final
{
public:
    [[nodiscard]] static QString defaultFilePath();
    [[nodiscard]] static std::optional<AppConfig> loadFromFile(const QString& filePath, QString& errorMessage);

    [[nodiscard]] const QString& pcIpAddress() const noexcept;
    [[nodiscard]] quint16 lhmPort() const noexcept;
    [[nodiscard]] int historyWindowSeconds() const noexcept;
    [[nodiscard]] int refreshPeriodMs() const noexcept;
    [[nodiscard]] int historySampleCapacity() const noexcept;

private:
    AppConfig(QString pcIpAddress, quint16 lhmPort, int historyWindowSeconds, int refreshPeriodMs, int historySampleCapacity);

    QString m_pcIpAddress;
    quint16 m_lhmPort;
    int m_historyWindowSeconds;
    int m_refreshPeriodMs;
    int m_historySampleCapacity;
};

} // namespace monitor

