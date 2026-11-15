// Kaeru — shared command handlers: UI, tray, CLI and DBus all call these
// (spec §12: "UI and CLI use same core command handlers")
#pragma once
#include <QString>
#include <QVariantMap>

namespace kaeru {

class Application;

struct CommandResult {
    bool ok = false;
    int exitCode = 0;      // 0 ok · 2 usage · 3 not running · 4 failed · 5 denied
    QString message;
    QVariantMap data;      // machine-readable payload (e.g. status json)

    static CommandResult success(const QString& message = QString(), const QVariantMap& data = QVariantMap())
    {
        CommandResult r;
        r.ok = true;
        r.message = message;
        r.data = data;
        return r;
    }
    static CommandResult failure(int code, const QString& message)
    {
        CommandResult r;
        r.ok = false;
        r.exitCode = code;
        r.message = message;
        return r;
    }
};

class CommandRouter final {
public:
    explicit CommandRouter(Application* app);

    // status [--json]
    CommandResult status(bool asJson) const;

    // awake on --duration 30m --mode system|display|both ; awake off
    CommandResult awakeOn(const QVariantMap& options);
    CommandResult awakeOff();
    CommandResult resumeAutomatic();

    // focus start|pause|resume|skip|stop|extend
    CommandResult focusCommand(const QString& verb, const QVariantMap& options);

    // work start|pause|resume|stop (recording wired in WP-05)
    CommandResult workCommand(const QString& verb, const QVariantMap& options);

    // pause-all / resume (unpause)
    CommandResult pauseAll();
    CommandResult resumeAll();

    // export (WP-06; errors until then)
    CommandResult exportCsv(const QVariantMap& options);

    // diagnostics [--json]
    CommandResult diagnostics(bool asJson) const;

    // open / quit
    void showWindowRequested();
    void quitRequested();

private:
    Application* m_app = nullptr;
};

} // namespace kaeru
