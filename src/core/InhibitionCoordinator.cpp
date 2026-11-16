#include "InhibitionCoordinator.h"

#include <limits>

namespace kaeru {

InhibitionCoordinator::InhibitionCoordinator(QObject* parent)
    : QObject(parent)
{
}

void InhibitionCoordinator::setAppState(AppState* state)
{
    m_state = state;
    if (m_state) {
        connect(m_state, &AppState::changed, this, [this] { resolve(nowUtcFallback()); });
        // Pause All interrupts active requests; interrupted manual sessions
        // are never auto-restarted when leaving Pause All (spec §7) — the
        // user restarts them explicitly.
        connect(m_state, &AppState::pausedAllChanged, this, [this](bool paused) {
            if (paused)
                m_registry.clear();
            resolve(nowUtcFallback());
        });
    }
}

void InhibitionCoordinator::addBackend(InhibitionBackend* backend, int priority)
{
    if (!backend)
        return;
    m_backends.append({ backend, priority });
    connect(backend, &InhibitionBackend::bindingActivated, this, &InhibitionCoordinator::onBindingActivated);
    connect(backend, &InhibitionBackend::bindingFailed, this, &InhibitionCoordinator::onBindingFailed);
    connect(backend, &InhibitionBackend::bindingLost, this, &InhibitionCoordinator::onBindingLost);
}

QUuid InhibitionCoordinator::addRequest(const InhibitionRequest& request)
{
    const QUuid id = m_registry.add(request);
    resolve(nowUtcFallback());
    return id;
}

bool InhibitionCoordinator::removeRequest(const QUuid& id)
{
    const bool removed = m_registry.remove(id);
    if (removed)
        resolve(nowUtcFallback());
    return removed;
}

void InhibitionCoordinator::suppressAll(const QString& reason)
{
    m_suppressed = true;
    m_suppressReason = reason;
    m_registry.clear();
    resolve(nowUtcFallback());
}

void InhibitionCoordinator::resumeAutomatic()
{
    m_suppressed = false;
    m_suppressReason.clear();
    resolve(nowUtcFallback());
}

void InhibitionCoordinator::releaseAll()
{
    m_registry.clear();
    resolve(nowUtcFallback());
}

void InhibitionCoordinator::tick(const QDateTime& nowUtc)
{
    m_wallNow = nowUtc;
    // Snapshot before expiry so the signal carries the original reason and
    // expiration details to the UI/notification layer.
    const QList<InhibitionRequest> before = m_registry.requests();
    const QList<QUuid> expiredIds = m_registry.expireElapsed(nowUtc);
    for (const QUuid& id : expiredIds) {
        InhibitionRequest expiredRequest;
        for (const InhibitionRequest& r : before) {
            if (r.id == id) {
                expiredRequest = r;
                break;
            }
        }
        emit expired(id, expiredRequest);
    }
    resolve(nowUtc);
}

Capabilities InhibitionCoordinator::desiredCapabilities() const
{
    Capabilities caps;
    for (const InhibitionRequest& r : m_registry.requests())
        caps |= r.capabilities;
    return caps;
}

InhibitionBackend* InhibitionCoordinator::pickBackend(Capability cap, int& triedPriority) const
{
    InhibitionBackend* best = nullptr;
    int bestPrio = INT_MAX;
    for (const BackendOrder& bo : m_backends) {
        if (bo.priority >= triedPriority)
            continue;
        if (bo.backend->supportedCapabilities().testFlag(cap) && bo.backend->serviceReachable()) {
            if (bo.priority < bestPrio) {
                best = bo.backend;
                bestPrio = bo.priority;
            }
        }
    }
    triedPriority = bestPrio;
    return best;
}

void InhibitionCoordinator::resolve(const QDateTime& nowUtc)
{
    if (m_resolving)
        return; // re-entrancy guard
    m_resolving = true;
    m_wallNow = nowUtc;
    const bool moduleActive = m_state ? (m_state->isModuleEnabled(Module::KeepAwake) && !m_state->isPausedAll()) : true;
    Capabilities desired;
    if (moduleActive && !m_suppressed)
        desired = desiredCapabilities();

    // 1. Release bindings for capabilities no longer desired.
    QList<QString> toRelease;
    for (auto it = m_bindings.begin(); it != m_bindings.end(); ++it) {
        const InhibitionBinding& b = it.value();
        const Capabilities keep = b.capabilities & desired;
        if (keep == 0)
            toRelease.append(it.key());
    }
    for (const QString& id : toRelease)
        releaseBinding(id);

    // 2. Acquire bindings for desired capabilities not yet covered.
    for (const quint32 capInt : { quint32(Capability::SystemSleep), quint32(Capability::SessionIdle) }) {
        const Capability cap = static_cast<Capability>(capInt);
        if (!desired.testFlag(cap))
            continue;
        bool covered = false;
        for (const InhibitionBinding& b : m_bindings) {
            if (b.capabilities.testFlag(cap) && (b.state == BindingState::Active || b.state == BindingState::Activating)) {
                covered = true;
                break;
            }
        }
        if (covered)
            continue;
        int triedPriority = std::numeric_limits<int>::max();
        InhibitionBackend* backend = pickBackend(cap, triedPriority);
        if (!backend) {
            // No backend can serve this capability — honest unsupported state.
            continue;
        }
        const QString bindingId = backend->acquire(Capabilities(cap), m_registry.primaryReason());
        InhibitionBinding binding;
        binding.id = bindingId;
        binding.capabilities = Capabilities(cap);
        binding.backendId = backend->backendId();
        binding.state = BindingState::Activating;
        m_bindings.insert(bindingId, binding);
    }

    m_resolving = false;
    emit stateChanged();
}

void InhibitionCoordinator::onBindingActivated(const QString& bindingId)
{
    if (!m_bindings.contains(bindingId))
        return;
    m_bindings[bindingId].state = BindingState::Active;
    m_bindings[bindingId].acquiredAt = QDateTime::currentDateTimeUtc();
    m_bindings[bindingId].errorDetail.clear();
    emit stateChanged();
}

void InhibitionCoordinator::onBindingFailed(const QString& bindingId, bool denied, const QString& detail)
{
    auto it = m_bindings.find(bindingId);
    if (it == m_bindings.end())
        return;
    const Capabilities caps = it->capabilities;
    const Capability cap = caps.testFlag(Capability::SystemSleep) ? Capability::SystemSleep : Capability::SessionIdle;
    const QString backendId = it->backendId;
    m_bindings.erase(it);

    // Try the next preferred backend for this capability.
    for (const BackendOrder& bo : m_backends) {
        if (bo.backend->backendId() == backendId)
            continue;
        if (!bo.backend->supportedCapabilities().testFlag(cap) || !bo.backend->serviceReachable())
            continue;
        const QString newId = bo.backend->acquire(Capabilities(cap), m_registry.primaryReason());
        InhibitionBinding nb;
        nb.id = newId;
        nb.capabilities = Capabilities(cap);
        nb.backendId = bo.backend->backendId();
        nb.state = BindingState::Activating;
        m_bindings.insert(newId, nb);
        emit stateChanged();
        return;
    }

    // No fallback available — record the failure for honest reporting.
    InhibitionBinding fb;
    fb.id = bindingId;
    fb.capabilities = caps;
    fb.backendId = backendId;
    fb.state = denied ? BindingState::Denied : BindingState::Failed;
    fb.errorDetail = detail;
    m_bindings.insert(bindingId, fb);
    emit stateChanged();
}

void InhibitionCoordinator::onBindingLost(const QString& bindingId, const QString& reason)
{
    auto it = m_bindings.find(bindingId);
    if (it == m_bindings.end())
        return;
    m_bindings.erase(it);
    // Re-resolve to re-acquire after backend loss (restart resilience).
    Q_UNUSED(reason)
    resolve(nowUtcFallback());
}

void InhibitionCoordinator::releaseBinding(const QString& bindingId)
{
    auto it = m_bindings.find(bindingId);
    if (it == m_bindings.end())
        return;
    const QString backendId = it->backendId;
    m_bindings.erase(it);
    for (const BackendOrder& bo : m_backends) {
        if (bo.backend->backendId() == backendId) {
            bo.backend->release(bindingId);
            break;
        }
    }
    emit stateChanged();
}

void InhibitionCoordinator::dropBinding(const QString& bindingId)
{
    m_bindings.remove(bindingId);
}

EffectiveAwakeState InhibitionCoordinator::effectiveState(const QDateTime& nowUtc) const
{
    EffectiveAwakeState s;
    // Reported "requested" is what the coordinator will actually act on:
    // module switch, Pause All and manual suppression gate the raw registry
    // (suppressed or disabled → nothing requested → Inactive, not Unsupported).
    const bool moduleActive = m_state
        ? (m_state->isModuleEnabled(Module::KeepAwake) && !m_state->isPausedAll())
        : true;
    s.requested = (moduleActive && !m_suppressed) ? desiredCapabilities() : Capabilities();
    if (m_suppressed) {
        s.limitation = m_suppressReason;
    }

    bool anyActive = false, anyActivating = false, anyFailed = false, anyDenied = false;
    QStringList backendNames;
    for (const InhibitionBinding& b : m_bindings) {
        switch (b.state) {
        case BindingState::Active:
            anyActive = true;
            s.effective |= b.capabilities;
            if (!backendNames.contains(b.backendId))
                backendNames.append(b.backendId);
            break;
        case BindingState::Activating:
            anyActivating = true;
            if (!backendNames.contains(b.backendId))
                backendNames.append(b.backendId);
            break;
        case BindingState::Failed:
            anyFailed = true;
            s.errorDetail = b.errorDetail;
            break;
        case BindingState::Denied:
            anyDenied = true;
            s.errorDetail = b.errorDetail.isEmpty() ? QStringLiteral("permission denied by backend") : b.errorDetail;
            break;
        case BindingState::Lost:
            anyActivating = true; // treated as re-acquiring
            break;
        }
    }

    if (anyActive && s.effective == s.requested)
        s.status = EffectiveStatus::Active;
    else if (anyActive)
        s.status = EffectiveStatus::PartiallyActive;
    else if (anyActivating)
        s.status = EffectiveStatus::Activating;
    else if (anyDenied)
        s.status = EffectiveStatus::PermissionDenied;
    else if (anyFailed)
        s.status = EffectiveStatus::Failed;
    else if (s.requested != 0)
        s.status = EffectiveStatus::Unsupported;
    else
        s.status = EffectiveStatus::Inactive;

    if (s.effective != s.requested && s.status == EffectiveStatus::PartiallyActive) {
        Capabilities missing = s.requested & ~s.effective;
        QStringList missingNames;
        if (missing.testFlag(Capability::SystemSleep))
            missingNames << capabilityName(Capability::SystemSleep);
        if (missing.testFlag(Capability::SessionIdle))
            missingNames << capabilityName(Capability::SessionIdle);
        s.limitation = QStringLiteral("not prevented: %1").arg(missingNames.join(QStringLiteral(", ")));
    }

    s.reason = m_registry.primaryReason();
    s.remainingMs = m_registry.earliestRemainingMs(nowUtc.isValid() ? nowUtc : m_wallNow);
    s.backend = backendNames.join(QStringLiteral("+"));
    return s;
}

QList<InhibitionBinding> InhibitionCoordinator::bindings() const
{
    return m_bindings.values();
}

QList<InhibitionRequest> InhibitionCoordinator::requests() const
{
    return m_registry.requests();
}

QDateTime InhibitionCoordinator::nowUtcFallback() const
{
    return m_wallNow.isValid() ? m_wallNow : QDateTime::currentDateTimeUtc();
}

} // namespace kaeru
