#include "LogindPowerMonitor.h"

#include <QDBusConnection>

namespace kaeru {

LogindPowerMonitor::LogindPowerMonitor(QObject* parent)
    : QObject(parent)
{
    QDBusConnection system = QDBusConnection::systemBus();
    m_available = system.connect(QString::fromUtf8("org.freedesktop.login1"),
        QString::fromUtf8("/org/freedesktop/login1"),
        QString::fromUtf8("org.freedesktop.login1.Manager"),
        QString::fromUtf8("PrepareForSleep"), this,
        SLOT(onPrepareForSleepSignal(bool)));
}

} // namespace kaeru
