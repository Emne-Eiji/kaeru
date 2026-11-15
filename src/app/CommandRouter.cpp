#include "CommandRouter.h"
#include "Application.h"
#include "Paths.h"

#include "core/Duration.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace kaeru {

CommandRouter::CommandRouter(Application* app)
    : m_app(app)
{
}

CommandResult CommandRouter::status(bool asJson) const
{
    const QVariantMap json = m_app->statusJson();
    if (asJson)
        return CommandResult::success(QString(), json);

    const QVariantMap ka = json.value(QStringLiteral("keepAwake")).toMap();
    QString line = QStringLiteral("Keep Awake: %1").arg(ka.value(QStringLiteral("status")).toString());
    const QString backend = ka.value(QStringLiteral("backend")).toString();
    if (!backend.isEmpty())
        line += QStringLiteral(" (%1)").arg(backend);
    const QVariant remaining = ka.value(QStringLiteral("remainingMs"));
    if (remaining.isValid() && remaining.toLongLong() >= 0)
        line += QStringLiteral(", remaining %1").arg(formatRemainingMs(remaining.toLongLong()));
    const QString reason = ka.value(QStringLiteral("reason")).toString();
    if (!reason.isEmpty())
        line += QStringLiteral("\nReason: %1").arg(reason);

    const QVariantMap focus = json.value(QStringLiteral("focus")).toMap();
    line += QStringLiteral("\nFocus: %1").arg(focus.value(QStringLiteral("phase")).toString());
    return CommandResult::success(line, json);
}

CommandResult CommandRouter::awakeOn(const QVariantMap& options)
{
    if (!m_app->appState()->isModuleEnabled(Module::KeepAwake))
        return CommandResult::failure(5, QStringLiteral("Keep Awake module is disabled (enable in Settings)"));

    // Manual suppression is cleared by an explicit user action to keep awake.
    m_app->resumeAutomaticRules();

    const QVariant duration = options.value(QStringLiteral("durationMs"));
    const QString mode = options.value(QStringLiteral("mode"), QStringLiteral("both")).toString();

    if (duration.isValid() && duration.toLongLong() > 0) {
        m_app->awakeOn(duration.toLongLong(), mode);
        return CommandResult::success(QStringLiteral("Keep awake requested (%1, %2) — activation confirmed asynchronously")
                .arg(mode, formatDurationMs(duration.toLongLong())));
    }
    m_app->awakeIndefinite(QStringLiteral("Keep awake (indefinite)"));
    return CommandResult::success(QStringLiteral("Keep awake requested (indefinite) — activation confirmed asynchronously"));
}

CommandResult CommandRouter::awakeOff()
{
    m_app->awakeOff();
    return CommandResult::success(QStringLiteral("Keep awake turned off; automatic requests are suppressed until you resume"));
}

CommandResult CommandRouter::resumeAutomatic()
{
    m_app->resumeAutomaticRules();
    return CommandResult::success(QStringLiteral("Automatic keep-awake rules resumed"));
}

CommandResult CommandRouter::focusCommand(const QString& verb, const QVariantMap& options)
{
    if (!m_app->appState()->isModuleEnabled(Module::Focus))
        return CommandResult::failure(5, QStringLiteral("Focus module is disabled"));
    FocusController* focus = m_app->focus();
    bool ok = false;
    if (verb == QStringLiteral("start")) {
        const QVariant duration = options.value(QStringLiteral("durationMs"));
        ok = duration.isValid() ? focus->startCustom(duration.toLongLong()) : focus->startPomodoro();
        if (!ok)
            return CommandResult::failure(4, QStringLiteral("Focus is already running (stop or skip first)"));
        return CommandResult::success(QStringLiteral("Focus started (%1)").arg(focus->phaseLabel()));
    }
    if (verb == QStringLiteral("pause")) {
        ok = focus->pause();
        return ok ? CommandResult::success(QStringLiteral("Focus paused"))
                  : CommandResult::failure(4, QStringLiteral("Focus is not running"));
    }
    if (verb == QStringLiteral("resume")) {
        ok = focus->resume();
        return ok ? CommandResult::success(QStringLiteral("Focus resumed"))
                  : CommandResult::failure(4, QStringLiteral("Focus is not paused"));
    }
    if (verb == QStringLiteral("skip")) {
        ok = focus->skipInterval();
        return ok ? CommandResult::success(QStringLiteral("Interval skipped; next: %1").arg(focus->phaseLabel()))
                  : CommandResult::failure(4, QStringLiteral("Focus is not running"));
    }
    if (verb == QStringLiteral("stop")) {
        ok = focus->stop();
        return ok ? CommandResult::success(QStringLiteral("Focus stopped"))
                  : CommandResult::failure(4, QStringLiteral("Focus is not running"));
    }
    if (verb == QStringLiteral("extend")) {
        ok = focus->extendInterval(options.value(QStringLiteral("durationMs"), 5 * 60 * 1000).toLongLong());
        return ok ? CommandResult::success(QStringLiteral("Interval extended"))
                  : CommandResult::failure(4, QStringLiteral("Focus is not running"));
    }
    return CommandResult::failure(2, QStringLiteral("unknown focus verb '%1' (start|pause|resume|skip|stop|extend)").arg(verb));
}

CommandResult CommandRouter::workCommand(const QString& verb, const QVariantMap& options)
{
    Q_UNUSED(options)
    if (!m_app->appState()->isModuleEnabled(Module::WorkLog))
        return CommandResult::failure(5, QStringLiteral("Work Log module is disabled"));
    // Work recording is delivered with WP-05; until then report honestly.
    Q_UNUSED(verb)
    return CommandResult::failure(4, QStringLiteral("Work Log recording is not available yet (delivered in a later milestone)"));
}

CommandResult CommandRouter::pauseAll()
{
    m_app->pauseAll();
    return CommandResult::success(QStringLiteral("Paused all: timers stopped, recording paused, inhibitors released"));
}

CommandResult CommandRouter::resumeAll()
{
    m_app->unpauseAll();
    return CommandResult::success(QStringLiteral("All modules available again (safe inactive state)"));
}

CommandResult CommandRouter::exportCsv(const QVariantMap& options)
{
    Q_UNUSED(options)
    return CommandResult::failure(4, QStringLiteral("Export is not available yet (delivered in a later milestone)"));
}

CommandResult CommandRouter::diagnostics(bool asJson) const
{
    const QVariantMap json = m_app->diagnosticsJson();
    if (asJson)
        return CommandResult::success(QString(), json);

    QStringList lines;
    lines << QStringLiteral("Kaeru %1 — capability diagnostics").arg(json.value(QStringLiteral("version")).toString());
    const QVariantMap caps = json.value(QStringLiteral("capabilities")).toMap();
    for (auto it = caps.constBegin(); it != caps.constEnd(); ++it) {
        const QVariantMap c = it.value().toMap();
        lines << QStringLiteral("  %1: %2 (%3)").arg(c.value(QStringLiteral("title")).toString(),
            c.value(QStringLiteral("status")).toString(),
            c.value(QStringLiteral("backend")).toString());
        const QString detail = c.value(QStringLiteral("detail")).toString();
        if (!detail.isEmpty())
            lines << QStringLiteral("      %1").arg(detail);
    }
    return CommandResult::success(lines.join(u'\n'), json);
}

void CommandRouter::showWindowRequested()
{
    emit m_app->showWindowRequested();
}

void CommandRouter::quitRequested()
{
    emit m_app->quitRequested();
}

} // namespace kaeru
