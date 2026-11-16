// Kaeru — D-Bus control service (spec §12, ADR-005)
// Interface io.github.kaeru.Kaeru.Control1 at /io/github/kaeru/Kaeru.
// Single instance + CLI IPC; session-bus scoped to the user's session.
#pragma once
#include <QDBusAbstractAdaptor>
#include <QString>
#include <QVariantMap>

namespace kaeru {

class Application;

class ControlService final : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.kaeru.Kaeru.Control1")

public:
    explicit ControlService(Application* app);

public slots:
    QString Status();
    QString Diagnostics();
    QString AwakeOn(double durationSeconds, const QString& mode);
    QString AwakeOff();
    QString ResumeAutomatic();
    QString FocusStart(double durationSeconds);
    QString FocusPause();
    QString FocusResume();
    QString FocusSkip();
    QString FocusStop();
    QString FocusExtend(double seconds);
    QString WorkStart(const QString& project);
    QString WorkPause();
    QString WorkResume();
    QString WorkStop();
    QString PauseAll();
    QString ResumeAll();
    QString ExportCsv(const QString& path, const QString& fromIso, const QString& toIso);
    void ShowWindow();
    void Quit();

signals:
    void showWindowRequested();
    void quitRequested();

private:
    Application* m_app;
};

} // namespace kaeru
