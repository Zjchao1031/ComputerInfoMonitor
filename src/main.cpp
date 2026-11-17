#include "mock/mock_monitor_data.h"
#include "ui/main_window.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringLiteral>
#include <QStringList>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("ComputerInfoMonitor"));
    QCoreApplication::setApplicationName(QStringLiteral("ComputerInfoMonitor"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("ComputerInfoMonitor Mock 界面"));
    parser.addHelpOption();
    const QCommandLineOption mockWithoutGpuOption(QStringList{QStringLiteral("mock-no-gpu")}, QStringLiteral("使用无独立显卡的 Mock 数据"));
    parser.addOption(mockWithoutGpuOption);
    parser.process(application);

    monitor::MainWindow window;
    if (parser.isSet(mockWithoutGpuOption))
    {
        const monitor::MonitorSnapshot snapshot = monitor::MockMonitorData::createSnapshot(false);
        window.setSnapshot(snapshot);
        window.setHistorySeries(monitor::MockMonitorData::createHistorySeries(snapshot));
    }

    window.show();

    return application.exec();
}

