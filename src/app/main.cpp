// Kaeru — entry point: single binary serving GUI, background and CLI modes
#include "Application.h"
#include "CliParser.h"
#include "CommandRouter.h"
#include "Paths.h"

#include "app/ControlService.h"
#include "platform/SingleInstance.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QTimer>
#include <iostream>

using namespace kaeru;

static int runCli(const CliInvocation& inv)
{
    int fakeArgc = 0;
    QCoreApplication app(fakeArgc, nullptr);
    QCoreApplication::setApplicationName(QStringLiteral("kaeru"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("io.github.kaeru"));

    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusInterface control(QStringLiteral(KAERU_APP_ID),
        QStringLiteral("/io/github/kaeru/Kaeru"),
        QStringLiteral("io.github.kaeru.Kaeru.Control1"), bus);
    if (!control.isValid()) {
        std::cout << "Kaeru is not running. Start it with: kaeru --background\n";
        return 3;
    }

    const auto call = [&control](const QString& method, const QVariantList& args = {}) -> int {
        QDBusReply<QString> reply = control.callWithArgumentList(QDBus::Block, method, args);
        if (!reply.isValid()) {
            std::cout << "ERROR: " << reply.error().message().toStdString() << "\n";
            return 4;
        }
        const QString value = reply.value();
        std::cout << value.toStdString() << "\n";
        if (value.startsWith(QStringLiteral("ERROR")))
            return value.startsWith(QStringLiteral("ERROR 5")) ? 5 : 4;
        return 0;
    };

    if (inv.command == QStringLiteral("status"))
        return call(QStringLiteral("Status"));
    if (inv.command == QStringLiteral("diagnostics"))
        return call(QStringLiteral("Diagnostics"));
    if (inv.command == QStringLiteral("awake")) {
        const QString verb = inv.operands.isEmpty() ? QString() : inv.operands.first();
        if (verb == QStringLiteral("on")) {
            const QVariant duration = inv.options.value(QStringLiteral("durationMs"));
            return call(QStringLiteral("AwakeOn"),
                { duration.isValid() ? duration.toLongLong() / 1000.0 : 0.0,
                    inv.options.value(QStringLiteral("mode"), QStringLiteral("both")).toString() });
        }
        if (verb == QStringLiteral("indefinite"))
            return call(QStringLiteral("AwakeOn"), { 0.0, QStringLiteral("both") });
        if (verb == QStringLiteral("off"))
            return call(QStringLiteral("AwakeOff"));
        std::cout << "usage: kaeru awake on --duration 30m --mode system|display|both\n"
                     "       kaeru awake indefinite\n"
                     "       kaeru awake off\n";
        return 2;
    }
    if (inv.command == QStringLiteral("focus") || inv.command == QStringLiteral("work")
        || inv.command == QStringLiteral("export") || inv.command == QStringLiteral("pause-all")) {
        std::cout << "This feature is not part of Kaeru " << KAERU_VERSION
                  << ". This release focuses on dependable Keep Awake.\n";
        return 2;
    }
    if (inv.command == QStringLiteral("diagnostics"))
        return call(QStringLiteral("Diagnostics"));
    if (inv.command == QStringLiteral("open")) {
        SingleInstance instance;
        return instance.activateExisting() ? 0 : 3;
    }
    if (inv.command == QStringLiteral("quit")) {
        // Quit returns void — use a raw message call, not QDBusReply<QString>.
        const QDBusMessage reply = control.call(QStringLiteral("Quit"));
        if (reply.type() == QDBusMessage::ErrorMessage) {
            std::cout << "ERROR: " << reply.errorMessage().toStdString() << "\n";
            return 4;
        }
        return 0;
    }

    return 2;
}

static void printHelp()
{
    std::cout << "Kaeru — dependable keep awake for your desktop\n\n"
                 "Usage:\n"
                 "  kaeru                       open the application window\n"
                 "  kaeru --background          start hidden; the tray owns the window\n"
                 "  kaeru status [--json]       show current Keep Awake status\n"
                 "  kaeru awake on --duration 30m --mode system|display|both\n"
                 "  kaeru awake indefinite      keep awake until turned off\n"
                 "  kaeru awake off             turn off now\n"
                 "  kaeru diagnostics [--json]  capability and backend details\n"
                 "  kaeru open | quit           show the window / quit a running Kaeru\n"
                 "  kaeru --version | --help\n\n"
                 "Durations accept 90s, 30m, 1h30m. Choosing a new duration replaces\n"
                 "the current request. Focus, Work Log and Reports are deferred and\n"
                 "not available in this release. Exit codes: 0 ok, 2 usage, 3 not\n"
                 "running, 4 failed, 5 denied.\n";
}

int main(int argc, char* argv[])
{
    // Honor test sandbox before any Qt path resolution.
    const QString testHome = qEnvironmentVariable("KAERU_TEST_HOME");
    if (!testHome.isEmpty())
        Paths::applyTestSandbox(testHome);

    QStringList rawArgs;
    for (int i = 1; i < argc; ++i)
        rawArgs << QString::fromLocal8Bit(argv[i]);

    const CliInvocation inv = CliParser::parse(rawArgs);
    if (inv.isUsageError()) {
        std::cerr << "kaeru: " << inv.error.toStdString() << "\n";
        std::cerr << "Try 'kaeru --help'.\n";
        return 2;
    }
    if (inv.mode == CliInvocation::Mode::Help) {
        printHelp();
        return 0;
    }
    if (inv.mode == CliInvocation::Mode::Version) {
        std::cout << "kaeru " << KAERU_VERSION << " (app id " << KAERU_APP_ID << ")\n";
        return 0;
    }
    if (inv.mode == CliInvocation::Mode::Command)
        return runCli(inv);

    // GUI mode (windowed or background)
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Kaeru"));
    QApplication::setOrganizationDomain(QStringLiteral("io.github.kaeru"));
    QApplication::setApplicationDisplayName(QStringLiteral("Kaeru"));
    QApplication::setDesktopFileName(QStringLiteral(KAERU_APP_ID));

    Application kaeruApp(inv.mode == CliInvocation::Mode::GuiBackground);

    SingleInstance instance;
    if (!instance.tryAcquire()) {
        // Another instance owns the bus name: forward and exit (spec §7).
        instance.activateExisting();
        return 0;
    }

    // Autostart entry: "--background --awake-on-login" starts silently and
    // immediately requests an indefinite Keep Awake session.
    if (inv.awakeOnLogin)
        kaeruApp.awakeIndefinite(QStringLiteral("Keep awake (started at login)"));

    MainWindow window(&kaeruApp);
    ControlService* control = new ControlService(&kaeruApp);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/io/github/kaeru/Kaeru"),
        &kaeruApp);
    QObject::connect(control, &ControlService::showWindowRequested,
        &window, &MainWindow::showWindow);
    QObject::connect(control, &ControlService::quitRequested, &kaeruApp, [ &kaeruApp ] {
        kaeruApp.prepareQuit();
        qApp->quit();
    });

    if (inv.mode == CliInvocation::Mode::GuiBackground) {
        // Tray/fallback installation is owned by MainWindow; window stays hidden.
        window.startHidden();
    } else {
        window.showWindow();
    }

    return app.exec();
}
