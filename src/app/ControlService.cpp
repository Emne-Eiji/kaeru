#include "ControlService.h"
#include "Application.h"
#include "CommandRouter.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QJsonDocument>
#include <QJsonObject>

namespace kaeru {

static QString toJson(const QVariantMap& map)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Compact));
}

static QString resultToString(const CommandResult& r)
{
    if (r.ok) {
        if (!r.message.isEmpty())
            return r.message;
        if (!r.data.isEmpty())
            return toJson(r.data);
        return QStringLiteral("ok");
    }
    return QStringLiteral("ERROR %1: %2").arg(r.exitCode).arg(r.message);
}

ControlService::ControlService(Application* app)
    : QDBusAbstractAdaptor(app)
    , m_app(app)
{
}

QString ControlService::Status()
{
    CommandRouter router(m_app);
    return resultToString(router.status(true));
}

QString ControlService::Diagnostics()
{
    CommandRouter router(m_app);
    return resultToString(router.diagnostics(true));
}

QString ControlService::AwakeOn(double durationSeconds, const QString& mode)
{
    CommandRouter router(m_app);
    QVariantMap options;
    if (durationSeconds > 0)
        options.insert(QStringLiteral("durationMs"), qint64(durationSeconds * 1000));
    options.insert(QStringLiteral("mode"), mode);
    return resultToString(router.awakeOn(options));
}

QString ControlService::AwakeOff()
{
    CommandRouter router(m_app);
    return resultToString(router.awakeOff());
}

QString ControlService::ResumeAutomatic()
{
    CommandRouter router(m_app);
    return resultToString(router.resumeAutomatic());
}

QString ControlService::FocusStart(double durationSeconds)
{
    CommandRouter router(m_app);
    QVariantMap options;
    if (durationSeconds > 0)
        options.insert(QStringLiteral("durationMs"), qint64(durationSeconds * 1000));
    return resultToString(router.focusCommand(QStringLiteral("start"), options));
}

QString ControlService::FocusPause()
{
    CommandRouter router(m_app);
    return resultToString(router.focusCommand(QStringLiteral("pause"), {}));
}

QString ControlService::FocusResume()
{
    CommandRouter router(m_app);
    return resultToString(router.focusCommand(QStringLiteral("resume"), {}));
}

QString ControlService::FocusSkip()
{
    CommandRouter router(m_app);
    return resultToString(router.focusCommand(QStringLiteral("skip"), {}));
}

QString ControlService::FocusStop()
{
    CommandRouter router(m_app);
    return resultToString(router.focusCommand(QStringLiteral("stop"), {}));
}

QString ControlService::FocusExtend(double seconds)
{
    CommandRouter router(m_app);
    QVariantMap options;
    options.insert(QStringLiteral("durationMs"), qint64(seconds * 1000));
    return resultToString(router.focusCommand(QStringLiteral("extend"), options));
}

QString ControlService::WorkStart(const QString& project)
{
    CommandRouter router(m_app);
    QVariantMap options;
    options.insert(QStringLiteral("project"), project);
    return resultToString(router.workCommand(QStringLiteral("start"), options));
}

QString ControlService::WorkPause()
{
    CommandRouter router(m_app);
    return resultToString(router.workCommand(QStringLiteral("pause"), {}));
}

QString ControlService::WorkResume()
{
    CommandRouter router(m_app);
    return resultToString(router.workCommand(QStringLiteral("resume"), {}));
}

QString ControlService::WorkStop()
{
    CommandRouter router(m_app);
    return resultToString(router.workCommand(QStringLiteral("stop"), {}));
}

QString ControlService::PauseAll()
{
    CommandRouter router(m_app);
    return resultToString(router.pauseAll());
}

QString ControlService::ResumeAll()
{
    CommandRouter router(m_app);
    return resultToString(router.resumeAll());
}

QString ControlService::ExportCsv(const QString& path, const QString& fromIso, const QString& toIso)
{
    CommandRouter router(m_app);
    QVariantMap options;
    options.insert(QStringLiteral("output"), path);
    options.insert(QStringLiteral("from"), fromIso);
    options.insert(QStringLiteral("to"), toIso);
    return resultToString(router.exportCsv(options));
}

void ControlService::ShowWindow()
{
    emit showWindowRequested();
}

void ControlService::Quit()
{
    emit quitRequested();
}

} // namespace kaeru
