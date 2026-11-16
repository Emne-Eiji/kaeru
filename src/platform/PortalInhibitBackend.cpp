#include "PortalInhibitBackend.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QUuid>

#include <cstdio>

namespace kaeru {

static constexpr const char* kPortalService = "org.freedesktop.portal.Desktop";
static constexpr const char* kPortalPath = "/org/freedesktop/portal/desktop";
static constexpr const char* kPortalInhibit = "org.freedesktop.portal.Inhibit";
static constexpr const char* kPortalRequest = "org.freedesktop.portal.Request";

PortalInhibitBackend::PortalInhibitBackend(QObject* parent)
    : InhibitionBackend(parent)
    , m_watcher(new QDBusServiceWatcher(QString::fromUtf8(kPortalService),
          QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this))
{
    connect(m_watcher, &QDBusServiceWatcher::serviceOwnerChanged,
        this, &PortalInhibitBackend::onServiceOwnerChanged);

    // Capability probe: portal present AND exposes Inhibit interface?
    const QDBusConnection bus = QDBusConnection::sessionBus();
    m_reachable = bus.interface()->isServiceRegistered(QString::fromUtf8(kPortalService));
    if (!m_reachable) {
        m_detail = QStringLiteral("portal service not running");
        return;
    }
    QDBusInterface inhibit(QString::fromUtf8(kPortalService), QString::fromUtf8(kPortalPath),
        QString::fromUtf8(kPortalInhibit), bus, this);
    if (!inhibit.isValid()) {
        m_reachable = false;
        m_detail = QStringLiteral("portal Inhibit interface unavailable");
        return;
    }
    m_detail = QStringLiteral("org.freedesktop.portal.Inhibit available");
}

Capabilities PortalInhibitBackend::supportedCapabilities() const
{
    if (!m_reachable)
        return {};
    // Flags 4 (Suspend) and 8 (Idle) are the spec's defined flags; a portal
    // that answers introspection is expected to support them. Actual
    // effectiveness is confirmed per-binding via the Response signal and
    // verified in integration tests; failures surface as Failed/Denied.
    return Capability::SystemSleep | Capability::SessionIdle;
}

quint32 PortalInhibitBackend::flagsFor(Capabilities caps) const
{
    quint32 flags = 0;
    if (caps.testFlag(Capability::SystemSleep))
        flags |= 4u;
    if (caps.testFlag(Capability::SessionIdle))
        flags |= 8u;
    return flags;
}

QString PortalInhibitBackend::acquire(Capabilities capabilities, const QString& reason)
{
    const QString bindingId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!m_reachable) {
        emit bindingFailed(bindingId, false, m_detail);
        return bindingId;
    }

    QDBusInterface inhibit(QString::fromUtf8(kPortalService), QString::fromUtf8(kPortalPath),
        QString::fromUtf8(kPortalInhibit), QDBusConnection::sessionBus(), this);
    if (!inhibit.isValid()) {
        emit bindingFailed(bindingId, false, QStringLiteral("portal Inhibit interface invalid"));
        return bindingId;
    }

    const QString token = QStringLiteral("kaeru_%1").arg(++m_nextToken);
    QVariantMap options;
    options.insert(QStringLiteral("handle_token"), token);
    options.insert(QStringLiteral("reason"), reason.isEmpty() ? QStringLiteral("Kaeru keep awake") : reason.left(120));

    // The portal may emit Response only milliseconds after Inhibit returns, so
    // subscribing after the async reply races and loses the confirmation
    // (observed: request held by the portal, app stuck "activating", release
    // unable to Close it). Predict the request handle per the portal spec and
    // subscribe before issuing the call:
    //   /org/freedesktop/portal/desktop/request/<sender-without-colon, dots→underscores>/<token>
    QString sender = QDBusConnection::sessionBus().baseService();
    if (sender.startsWith(QLatin1Char(':')))
        sender.remove(0, 1);
    sender.replace(QLatin1Char('.'), QLatin1Char('_'));
    const QString expectedHandle = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2")
        .arg(sender, token);

    PendingBinding pb;
    pb.id = bindingId;
    pb.handlePath = expectedHandle;
    pb.capabilities = capabilities;
    pb.reason = reason;
    m_pending.insert(expectedHandle, pb);
    subscribeResponse(expectedHandle, bindingId);

    // window identifier: empty for a tray/background utility (validated live).
    QDBusPendingReply<QDBusObjectPath> reply =
        inhibit.asyncCall(QStringLiteral("Inhibit"), QString(), flagsFor(capabilities), options);
    auto* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, bindingId, capabilities, reason, expectedHandle](QDBusPendingCallWatcher* call) {
        QDBusPendingReply<QDBusObjectPath> reply = *call;
        call->deleteLater();
        if (reply.isError()) {
            m_pending.remove(expectedHandle);
            emit bindingFailed(bindingId, false, QStringLiteral("portal call failed: %1").arg(reply.error().message()));
            return;
        }
        const QString handlePath = reply.value().path();
        if (handlePath.isEmpty()) {
            m_pending.remove(expectedHandle);
            emit bindingFailed(bindingId, false, QStringLiteral("portal returned empty handle"));
            return;
        }
        if (handlePath != expectedHandle) {
            // Portal did not honor handle_token; re-key and subscribe to the
            // actual handle. A Response may already have been emitted for it
            // and is unrecoverable, but this at least keeps release working.
            if (m_pending.contains(expectedHandle)) {
                PendingBinding moved = m_pending.take(expectedHandle);
                moved.handlePath = handlePath;
                m_pending.insert(handlePath, moved);
            }
            subscribeResponse(handlePath, bindingId);
        }
    });
    return bindingId;
}

void PortalInhibitBackend::subscribeResponse(const QString& handlePath, const QString& bindingId)
{
    // The Response signal confirms activation (0), user denial (1), or failure (2).
    const bool ok = QDBusConnection::sessionBus().connect(QString::fromUtf8(kPortalService), handlePath,
        QString::fromUtf8(kPortalRequest), QStringLiteral("Response"),
        this, SLOT(onResponse(uint, QVariantMap)));
    if (qEnvironmentVariableIsSet("KAERU_DEBUG_PORTAL"))
        std::fprintf(stderr, "[kaeru-portal] subscribe handle=%s ok=%d\n",
                     qPrintable(handlePath), ok ? 1 : 0);
    Q_UNUSED(bindingId)
}

void PortalInhibitBackend::onResponse(const uint responseCode, const QVariantMap& results)
{
    Q_UNUSED(results)
    // QDBusContext::message() identifies which request handle dispatched
    // this Response signal (0 = success, 1 = denied/cancelled, 2 = failed).
    const QString handlePath = message().path();
    if (qEnvironmentVariableIsSet("KAERU_DEBUG_PORTAL"))
        std::fprintf(stderr, "[kaeru-portal] response code=%u path='%s' pending=%d\n",
                     responseCode, qPrintable(handlePath), int(m_pending.size()));
    if (!m_pending.contains(handlePath)) {
        // No matching request: ignore rather than mis-attributing the signal
        // to another in-flight request.
        return;
    }
    const PendingBinding pb = m_pending.take(handlePath);
    if (responseCode == 0) {
        m_active.insert(pb.id, handlePath);
        emit bindingActivated(pb.id);
    } else if (responseCode == 1) {
        emit bindingFailed(pb.id, true, QStringLiteral("portal response: request denied/cancelled"));
    } else {
        emit bindingFailed(pb.id, false, QStringLiteral("portal response: failure (code %1)").arg(responseCode));
    }
}

void PortalInhibitBackend::release(const QString& bindingId)
{
    QString handlePath = m_active.take(bindingId);
    if (handlePath.isEmpty()) {
        // The request may still be activating: find and forget it, then Close
        // the predicted handle so the portal does not keep an orphan inhibitor.
        for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
            if (it->id == bindingId) {
                handlePath = it.key();
                m_pending.erase(it);
                break;
            }
        }
    }
    if (handlePath.isEmpty())
        return;
    QDBusInterface request(QString::fromUtf8(kPortalService), handlePath,
        QString::fromUtf8(kPortalRequest), QDBusConnection::sessionBus(), this);
    if (request.isValid())
        request.asyncCall(QStringLiteral("Close"));
}

void PortalInhibitBackend::onServiceOwnerChanged(const QString& name, const QString& oldOwner, const QString& newOwner)
{
    Q_UNUSED(oldOwner)
    if (name != QString::fromUtf8(kPortalService))
        return;
    const bool nowRunning = !newOwner.isEmpty();
    m_reachable = nowRunning;
    if (!nowRunning) {
        markLost(QStringLiteral("portal service stopped"));
    } else {
        m_detail = QStringLiteral("portal service restarted");
        // Coordinator re-resolves on bindingLost; no immediate re-acquire here.
    }
}

void PortalInhibitBackend::markLost(const QString& reason)
{
    const QStringList ids = m_active.keys();
    for (const QString& id : ids) {
        m_active.remove(id);
        emit bindingLost(id, reason);
    }
    m_pending.clear();
}

} // namespace kaeru
