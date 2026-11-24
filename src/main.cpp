#include "core/app_config.h"
#include "core/monitor_service.h"
#include "ui/main_window.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QFont>
#include <QStringLiteral>

#ifdef Q_OS_UNIX
#include <QSocketNotifier>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <cstdlib>
#include <optional>

namespace
{

#ifdef Q_OS_UNIX
class PosixSignalBridge final
{
public:
    PosixSignalBridge()
    {
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair) != 0)
            return;

        m_notifier = new QSocketNotifier(m_socketPair[1], QSocketNotifier::Read, QCoreApplication::instance());
        QObject::connect(m_notifier, &QSocketNotifier::activated, m_notifier, [this](QSocketDescriptor, QSocketNotifier::Type)
        {
            char signalByte = 0;
            const ssize_t bytesRead = ::read(m_socketPair[1], &signalByte, sizeof(signalByte));
            Q_UNUSED(bytesRead)
            QCoreApplication::quit();
        });

        s_instance = this;
        std::signal(SIGHUP, handleSignal);
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);
    }

    ~PosixSignalBridge()
    {
        std::signal(SIGHUP, SIG_DFL);
        std::signal(SIGINT, SIG_DFL);
        std::signal(SIGTERM, SIG_DFL);

        if (s_instance == this)
            s_instance = nullptr;

        if (m_notifier != nullptr)
            m_notifier->setEnabled(false);

        for (int descriptor : m_socketPair)
        {
            if (descriptor >= 0)
                ::close(descriptor);
        }
    }

private:
    static void handleSignal(int signalNumber)
    {
        if (s_instance == nullptr)
            return;

        const char signalByte = static_cast<char>(signalNumber);
        const ssize_t bytesWritten = ::write(s_instance->m_socketPair[0], &signalByte, sizeof(signalByte));
        Q_UNUSED(bytesWritten)
    }

    inline static PosixSignalBridge* s_instance{nullptr};
    int m_socketPair[2]{-1, -1};
    QSocketNotifier* m_notifier{nullptr};
};
#endif

} // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QFont font(QStringLiteral("WenQuanYi Zen Hei"));
    font.setStyleStrategy(QFont::PreferAntialias);
    QApplication::setFont(font);

    QCoreApplication::setOrganizationName(QStringLiteral("ComputerInfoMonitor"));
    QCoreApplication::setApplicationName(QStringLiteral("ComputerInfoMonitor"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QString configError;
    const std::optional<monitor::AppConfig> config = monitor::AppConfig::loadFromFile(monitor::AppConfig::defaultFilePath(), configError);
    if (!config.has_value())
    {
        qCritical().noquote() << QStringLiteral("配置错误：%1").arg(configError);
        return EXIT_FAILURE;
    }

    monitor::MainWindow window;
    monitor::MonitorService service(*config, &window);
    QObject::connect(&service, &monitor::MonitorService::snapshotUpdated, &window, &monitor::MainWindow::setSnapshot);
    QObject::connect(&service, &monitor::MonitorService::historyUpdated, &window, &monitor::MainWindow::setHistorySeries);
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &service, &monitor::MonitorService::stop);
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &window, &monitor::MainWindow::prepareForShutdown);

#ifdef Q_OS_UNIX
    PosixSignalBridge signalBridge;
#endif

    window.show();
    service.start();

    return application.exec();
}

