#include "CommandRouter.h"
#include "Application.h"
#include "Paths.h"
#include "WorkSessionManager.h"
#include "storage/CsvExport.h"

#include "core/Duration.h"

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QStandardPaths>
#include <QTime>

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
    if (!m_app->appState()->isModuleEnabled(Module::WorkLog))
        return CommandResult::failure(5, QStringLiteral("Work Log module is disabled"));
    if (!m_app->storageReady() || !m_app->work())
        return CommandResult::failure(4, QStringLiteral("Work Log storage is unavailable (see Diagnostics)"));
    WorkSessionManager* w = m_app->work();
    QString err;
    if (verb == QStringLiteral("start")) {
        if (!w->start(options.value(QStringLiteral("project")).toString(), &err))
            return CommandResult::failure(4, err);
        return CommandResult::success(QStringLiteral("Work Log recording started"));
    }
    if (verb == QStringLiteral("pause")) {
        if (!w->pause(QStringLiteral("manual"), &err))
            return CommandResult::failure(4, err);
        return CommandResult::success(QStringLiteral("Work Log recording paused"));
    }
    if (verb == QStringLiteral("resume")) {
        if (!w->resume(&err))
            return CommandResult::failure(4, err);
        return CommandResult::success(QStringLiteral("Work Log recording resumed"));
    }
    if (verb == QStringLiteral("stop")) {
        if (!w->stop(&err))
            return CommandResult::failure(4, err);
        return CommandResult::success(QStringLiteral("Work Log recording stopped and saved"));
    }
    return CommandResult::failure(2, QStringLiteral("unknown work verb '%1' (start|pause|resume|stop)").arg(verb));
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
    if (!m_app->storageReady() || !m_app->sessions())
        return CommandResult::failure(4, QStringLiteral("Work Log storage is unavailable"));
    // Dates were normalized by the CLI parser (YYYY-MM-DD) or come from the
    // GUI as ISO strings.
    bool okFrom = false, okTo = false;
    const QDate from = QDate::fromString(options.value(QStringLiteral("from")).toString(),
                                         Qt::ISODate);
    const QDate to = QDate::fromString(options.value(QStringLiteral("to")).toString(),
                                       Qt::ISODate);
    okFrom = from.isValid();
    okTo = to.isValid();
    if (!okFrom || !okTo)
        return CommandResult::failure(2, QStringLiteral("--from/--to must be YYYY-MM-DD"));
    if (from > to)
        return CommandResult::failure(2, QStringLiteral("--from must not be after --to"));

    const QDateTime fromUtc = QDateTime(from, QTime(0, 0), Qt::UTC);
    const QDateTime toUtc = QDateTime(to, QTime(23, 59, 59, 999), Qt::UTC);

    QString path = options.value(QStringLiteral("output")).toString();
    if (path.isEmpty())
        path = QDir::current().filePath(CsvExport::fileNameFor(from, to));

    QString err;
    const QList<CsvExport::Row> rows =
        CsvExport::gather(*m_app->sessions(), *m_app->projects(), fromUtc, toUtc, &err);
    if (!err.isEmpty())
        return CommandResult::failure(4, err);
    int rowCount = 0;
    if (!CsvExport::write(CsvExport::serialize(rows), path, &err, &rowCount))
        return CommandResult::failure(4, err);
    return CommandResult::success(
        QStringLiteral("Exported %1 row(s) to %2").arg(rowCount).arg(path));
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
