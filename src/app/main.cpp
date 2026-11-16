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
        if (verb == QStringLiteral("on"))
            return call(QStringLiteral("AwakeOn"),
                { inv.options.value(QStringLiteral("durationMs"), 0.0).toLongLong() / 1000.0,
                    inv.options.value(QStringLiteral("mode"), QStringLiteral("both")).toString() });
        if (verb == QStringLiteral("off"))
            return call(QStringLiteral("AwakeOff"));
        std::cout << "usage: kaeru awake on --duration 30m --mode system|display|both\n"
                     "       kaeru awake off\n";
        return 2;
    }
    if (inv.command == QStringLiteral("focus")) {
        const QString verb = inv.operands.isEmpty() ? QString() : inv.operands.first();
        if (verb == QStringLiteral("start"))
            return call(QStringLiteral("FocusStart"),
                { inv.options.value(QStringLiteral("durationMs"), 0.0).toLongLong() / 1000.0 });
        if (verb == QStringLiteral("pause"))
            return call(QStringLiteral("FocusPause"));
        if (verb == QStringLiteral("resume"))
            return call(QStringLiteral("FocusResume"));
        if (verb == QStringLiteral("skip"))
            return call(QStringLiteral("FocusSkip"));
        if (verb == QStringLiteral("stop"))
            return call(QStringLiteral("FocusStop"));
        if (verb == QStringLiteral("extend"))
            return call(QStringLiteral("FocusExtend"),
                { inv.options.value(QStringLiteral("durationMs"), 5 * 60 * 1000.0).toLongLong() / 1000.0 });
        std::cout << "usage: kaeru focus start|pause|resume|skip|stop|extend [--duration 25m]\n";
        return 2;
    }
    if (inv.command == QStringLiteral("work")) {
        const QString verb = inv.operands.isEmpty() ? QString() : inv.operands.first();
        if (verb == QStringLiteral("start"))
            return call(QStringLiteral("WorkStart"), { inv.options.value(QStringLiteral("project")).toString() });
        if (verb == QStringLiteral("pause"))
            return call(QStringLiteral("WorkPause"));
        if (verb == QStringLiteral("resume"))
            return call(QStringLiteral("WorkResume"));
        if (verb == QStringLiteral("stop"))
            return call(QStringLiteral("WorkStop"));
        std::cout << "usage: kaeru work start --project \"Example\" | pause | resume | stop\n";
        return 2;
    }
    if (inv.command == QStringLiteral("export")) {
        return call(QStringLiteral("ExportCsv"),
            { inv.options.value(QStringLiteral("output")).toString(),
                inv.options.value(QStringLiteral("from")).toString(),
                inv.options.value(QStringLiteral("to")).toString() });
    }
    if (inv.command == QStringLiteral("pause-all"))
        return call(QStringLiteral("PauseAll"));
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
    std::cout << "Kaeru — keep awake, focus and work log utility\n\n"
                 "Usage:\n"
                 "  kaeru                       open the application window\n"
                 "  kaeru --background          start without showing the window\n"
                 "  kaeru status [--json]       show current status\n"
                 "  kaeru awake on --duration 30m --mode system|display|both\n"
                 "  kaeru awake off             turn off now (suppresses automatic requests)\n"
                 "  kaeru focus start [--duration 25m] | pause | resume | skip | extend | stop\n"
                 "  kaeru work start --project \"Example\" | pause | resume | stop\n"
                 "  kaeru export --format csv --from 2026-11-01 --to 2026-11-30 [--output FILE]\n"
                 "  kaeru pause-all | diagnostics [--json] | open | quit\n"
                 "  kaeru --version | --help\n\n"
                 "Durations accept 90s, 30m, 1h30m. Exit codes: 0 ok, 2 usage, 3 not running,\n"
                 "4 failed, 5 denied.\n";
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
