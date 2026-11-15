#include "AutostartBackend.h"
#include "app/Paths.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

namespace kaeru {

AutostartBackend::AutostartBackend(QObject* parent)
    : QObject(parent)
{
}

QString AutostartBackend::autostartDir() const
{
    const QString configHome = Paths::configHome();
    return configHome + QStringLiteral("/autostart");
}

QString AutostartBackend::entryPath() const
{
    return autostartDir() + QStringLiteral("/io.github.kaeru.Kaeru.desktop");
}

bool AutostartBackend::isEnabled() const
{
    return QFile::exists(entryPath());
}

bool AutostartBackend::enable(const QString& extraArgs)
{
    QDir dir;
    if (!dir.mkpath(autostartDir())) {
        m_lastError = QStringLiteral("cannot create autostart directory");
        return false;
    }
    QFile f(entryPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastError = QStringLiteral("cannot write autostart entry");
        return false;
    }
    QTextStream ts(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ts.setCodec("UTF-8");
#endif
    const QString exec = QStringLiteral("kaeru%1").arg(extraArgs.isEmpty() ? QString() : QStringLiteral(" ") + extraArgs);
    ts << QStringLiteral("[Desktop Entry]\n")
       << QStringLiteral("Type=Application\n")
       << QStringLiteral("Name=Kaeru\n")
       << QStringLiteral("Comment=Keep awake, focus and work log\n")
       << QStringLiteral("Exec=%1\n").arg(exec)
       << QStringLiteral("Icon=io.github.kaeru.Kaeru\n")
       << QStringLiteral("Terminal=false\n")
       << QStringLiteral("Categories=Utility;\n")
       << QStringLiteral("X-GNOME-Autostart-enabled=true\n")
       << QStringLiteral("StartupNotify=false\n");
    f.close();
    m_lastError.clear();
    return true;
}

bool AutostartBackend::disable()
{
    QFile f(entryPath());
    if (!f.exists()) {
        m_lastError.clear();
        return true;
    }
    if (!f.remove()) {
        m_lastError = QStringLiteral("cannot remove autostart entry");
        return false;
    }
    m_lastError.clear();
    return true;
}

} // namespace kaeru
