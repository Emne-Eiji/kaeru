#include "LockMonitor.h"

#include <QDBusConnection>

namespace kaeru {

LockMonitor::LockMonitor(QObject* parent)
    : QObject(parent)
{
    QDBusConnection session = QDBusConnection::sessionBus();

    const bool gnome = session.connect(QString::fromUtf8("org.gnome.ScreenSaver"),
        QString::fromUtf8("/org/gnome/ScreenSaver"),
        QString::fromUtf8("org.gnome.ScreenSaver"),
        QString::fromUtf8("ActiveChanged"), this, SLOT(onGnomeActiveChanged(bool)));
    if (gnome) {
        m_available = true;
        m_source = QStringLiteral("org.gnome.ScreenSaver");
        return;
    }

    // logind path: watch LockedHint in PropertiesChanged of the session object.
    const bool logind = session.connect(QString::fromUtf8("org.freedesktop.login1"),
        QString::fromUtf8("/org/freedesktop/login1/session/self"),
        QString::fromUtf8("org.freedesktop.DBus.Properties"),
        QString::fromUtf8("PropertiesChanged"), this,
        SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
    if (logind) {
        m_available = true;
        m_source = QStringLiteral("logind LockedHint");
    }
}

void LockMonitor::onGnomeActiveChanged(bool active)
{
    m_locked = active;
    emit lockStateChanged(active);
}

void LockMonitor::onPropertiesChanged(const QString& interface, const QVariantMap& changed, const QStringList& invalidated)
{
    Q_UNUSED(invalidated)
    if (interface != QStringLiteral("org.freedesktop.login1.Session"))
        return;
    if (changed.contains(QStringLiteral("LockedHint"))) {
        m_locked = changed.value(QStringLiteral("LockedHint")).toBool();
        emit lockStateChanged(m_locked);
    }
}

} // namespace kaeru
