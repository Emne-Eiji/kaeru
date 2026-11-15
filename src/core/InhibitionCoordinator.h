// Kaeru — inhibition coordinator (spec §6.1/§6.2, ADR-002)
// Resolves registry requests into backend bindings with explicit precedence:
//   1. Quit / module disable / Pause All
//   2. Safety constraints (never shutdown/logout/lid; not implemented here —
//      backends only expose sleep/idle capabilities by design)
//   3. Explicit "turn off now" suppression
//   4. Eligible requests
#pragma once
#include "AppState.h"
#include "InhibitionBackend.h"
#include "InhibitionRegistry.h"
#include "Types.h"

#include <QDateTime>
#include <QHash>
#include <QObject>

namespace kaeru {

class InhibitionCoordinator final : public QObject {
    Q_OBJECT
public:
    struct BackendEntry {
        InhibitionBackend* backend = nullptr;
        int priority = 0; // lower = preferred
    };

    explicit InhibitionCoordinator(QObject* parent = nullptr);

    void setAppState(AppState* state);
    void addBackend(InhibitionBackend* backend, int priority);

    // Mutations (called by CommandRouter / timers / focus / work modules)
    QUuid addRequest(const InhibitionRequest& request);
    bool removeRequest(const QUuid& id);
    void suppressAll(const QString& reason);    // "Turn off Keep Awake now"
    void resumeAutomatic();                     // clear suppression
    void releaseAll();                          // module disable / quit path

    // Periodic tick (expiry checks + re-resolve); called by TimerEngine or a QTimer.
    void tick(const QDateTime& nowUtc);

    EffectiveAwakeState effectiveState(const QDateTime& nowUtc) const;
    QList<InhibitionBinding> bindings() const;
    QList<InhibitionRequest> requests() const;
    bool suppressed() const { return m_suppressed; }

signals:
    void stateChanged();      // any change to requested/effective state
    void expired(const QUuid& requestId, const InhibitionRequest& request);

private slots:
    void onBindingActivated(const QString& bindingId);
    void onBindingFailed(const QString& bindingId, bool denied, const QString& detail);
    void onBindingLost(const QString& bindingId, const QString& reason);

private:
    void resolve(const QDateTime& nowUtc);
    Capabilities desiredCapabilities() const;
    InhibitionBackend* pickBackend(Capability cap, int& triedPriority) const;
    void releaseBinding(const QString& bindingId);
    void dropBinding(const QString& bindingId);
    QDateTime nowUtcFallback() const;

    AppState* m_state = nullptr;
    InhibitionRegistry m_registry;
    struct BackendOrder {
        InhibitionBackend* backend;
        int priority;
    };
    QList<BackendOrder> m_backends;

    QHash<QString, InhibitionBinding> m_bindings;   // bindingId → binding
    QHash<QString, QUuid> m_bindingToRequest;       // bindingId → requestId
    QHash<QUuid, QList<QString>> m_requestToBinding; // requestId → bindingIds

    bool m_suppressed = false;
    QString m_suppressReason;
    QDateTime m_wallNow; // wall clock provided by tick() for expiry checks
    bool m_resolving = false;
};

} // namespace kaeru
