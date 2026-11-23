#include "ui/theme.h"

#include <QColor>
#include <QLatin1String>
#include <QStringLiteral>

namespace monitor::theme
{

QString styleSheet()
{
    QString result = QStringLiteral(R"(
QWidget#centralWidget {
    background-color: @background;
    background-image: url(:/images/interface-background.png);
    background-position: center;
    background-repeat: no-repeat;
    color: @textPrimary;
}
QWidget {
    color: @textPrimary;
}
QStackedWidget#pageStack,
QWidget#page {
    background-color: transparent;
}
QWidget#statusHeader {
    background-color: transparent;
}
QLabel#pageTitle {
    color: @accentYellow;
    font-size: 24px;
    font-weight: 700;
}
QFrame#monitorCard {
    background-color: transparent;
    border: 1px solid transparent;
    border-radius: 8px;
}
QFrame#monitorCard[interactive="true"]:hover {
    background-color: transparent;
    border: 1px solid rgba(255, 213, 79, 0.82);
}
QLabel#cardTitle {
    color: @accentCyan;
    font-size: 16px;
    font-weight: 700;
}
QLabel#hardwareName {
    color: @textPrimary;
    font-size: 15px;
    font-weight: 600;
}
QFrame#cardSeparator {
    background-color: rgba(88, 217, 255, 0.24);
    border: none;
    min-height: 1px;
    max-height: 1px;
}
QLabel#metricLabel {
    color: @textSecondary;
    font-size: 13px;
}
QLabel#metricValue {
    color: @textPrimary;
    font-size: 18px;
    font-weight: 600;
}
QLabel#metricUnit {
    color: @textSecondary;
    font-size: 12px;
    font-weight: 500;
}
QFrame#monitorCard QLabel#cardTitle {
    font-size: 18px;
}
QFrame#monitorCard QLabel#hardwareName {
    font-size: 17px;
}
QFrame#monitorCard QLabel#metricLabel {
    font-size: 16px;
}
QFrame#monitorCard QLabel#metricValue {
    font-size: 24px;
    font-weight: 700;
}
QFrame#monitorCard QLabel#metricUnit {
    font-size: 14px;
    font-weight: 600;
}
QLabel#memoryValue {
    color: @textPrimary;
    font-size: 26px;
    font-weight: 700;
}
QFrame#powerStrip {
    background-color: transparent;
    border: none;
}
QLabel#powerTitle {
    color: @textSecondary;
    font-size: 15px;
    font-weight: 600;
}
QLabel#powerValue {
    color: @textPrimary;
    font-size: 27px;
    font-weight: 700;
}
QLabel#powerUnit {
    color: @textSecondary;
    font-size: 15px;
    font-weight: 600;
}
QLabel#connectionText {
    color: @textPrimary;
    font-size: 15px;
    font-weight: 600;
}
QLabel#connectionDot {
    border-radius: 6px;
}
QLabel#connectionDot[connectionTone="starting"] {
    background-color: @accentCyan;
}
QLabel#connectionDot[connectionTone="online"] {
    background-color: @online;
}
QLabel#connectionDot[connectionTone="warning"] {
    background-color: @delayed;
}
QLabel#connectionDot[connectionTone="critical"] {
    background-color: @offline;
}
QLabel#connectionDot[connectionTone="unavailable"] {
    background-color: @textSecondary;
}
QLabel#lastUpdated {
    color: @textSecondary;
    font-size: 13px;
}
QToolButton#backButton {
    background-color: transparent;
    border: none;
    color: @accentYellow;
    font-size: 14px;
    font-weight: 600;
    padding: 0;
}
QToolButton#backButton:hover {
    color: @accentCyan;
}
QLabel#detailTitle {
    color: @textPrimary;
    font-size: 20px;
    font-weight: 700;
}
QLabel#memoryDetailValue {
    color: @accentYellow;
    font-size: 26px;
    font-weight: 700;
}
QListView#sensorTree {
    background-color: transparent;
    border: none;
    color: @textPrimary;
    font-size: 12px;
    outline: none;
}
QListView#sensorTree QScrollBar:vertical {
    background-color: transparent;
    border: none;
    width: 6px;
    margin: 0;
}
QListView#sensorTree QScrollBar::handle:vertical {
    background-color: rgba(88, 217, 255, 0.45);
    border-radius: 3px;
    min-height: 24px;
}
QListView#sensorTree QScrollBar::add-line:vertical,
QListView#sensorTree QScrollBar::sub-line:vertical {
    height: 0;
}
)");

    result.replace(QLatin1String("@background"), QLatin1String(kBackground));
    result.replace(QLatin1String("@textPrimary"), QLatin1String(kTextPrimary));
    result.replace(QLatin1String("@textSecondary"), QLatin1String(kTextSecondary));
    result.replace(QLatin1String("@accentYellow"), QLatin1String(kAccentYellow));
    result.replace(QLatin1String("@accentCyan"), QLatin1String(kAccentCyan));
    result.replace(QLatin1String("@online"), QLatin1String(kOnline));
    result.replace(QLatin1String("@delayed"), QLatin1String(kDelayed));
    result.replace(QLatin1String("@offline"), QLatin1String(kOffline));
    return result;
}

QString connectionStateText(ConnectionState state)
{
    switch (state)
    {
    case ConnectionState::Starting:
        return QStringLiteral("正在连接");
    case ConnectionState::Online:
        return QStringLiteral("在线");
    case ConnectionState::Delayed:
        return QStringLiteral("数据延迟");
    case ConnectionState::Offline:
        return QStringLiteral("PC 离线");
    case ConnectionState::NetworkUnavailable:
        return QStringLiteral("网络未连接");
    case ConnectionState::ServiceUnavailable:
        return QStringLiteral("LHM 不可用");
    case ConnectionState::ParseError:
        return QStringLiteral("数据格式错误");
    }

    return QStringLiteral("状态未知");
}

} // namespace monitor::theme

