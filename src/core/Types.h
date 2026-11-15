// Kaeru — shared core types (spec §6.1/§6.2)
#pragma once
#include <QDateTime>
#include <QFlags>
#include <QObject>
#include <QRect>
#include <QString>
#include <QUuid>
#include <QVariantMap>

namespace kaeru {

constexpr const char* kAppId = KAERU_APP_ID;
constexpr const char* kAppName = "Kaeru";

// Capabilities
enum class Capability : quint8 { SystemSleep = 0x1, SessionIdle = 0x2 };
Q_DECLARE_FLAGS(Capabilities, Capability)
Q_DECLARE_OPERATORS_FOR_FLAGS(Capabilities)

inline QString capabilityName(Capability c)
{
    switch (c) {
    case Capability::SystemSleep: return QStringLiteral("system-sleep");
    case Capability::SessionIdle: return QStringLiteral("session-idle");
    }
    return QString();
}

// Request sources (schedules/commands deferred per spec §6.2)
enum class RequestSource : quint8 { Manual, Countdown, Focus, WorkSession };

inline QString requestSourceName(RequestSource s)
{
    switch (s) {
    case RequestSource::Manual: return QStringLiteral("manual");
    case RequestSource::Countdown: return QStringLiteral("countdown");
    case RequestSource::Focus: return QStringLiteral("focus");
    case RequestSource::WorkSession: return QStringLiteral("work-session");
    }
    return QString();
}

// Expiration
enum class ExpirationPolicy : quint8 { None, Duration, UntilTime };

// Per-request status
enum class RequestState : quint8 { Pending, Active, Expired, Released, Suppressed };

// Effective, user-visible status (never unqualified "Active" unless confirmed)
enum class EffectiveStatus : quint8 {
    Inactive,
    Activating,
    Active,
    PartiallyActive,
    Unsupported,
    PermissionDenied,
    Failed
};

inline QString effectiveStatusName(EffectiveStatus s)
{
    switch (s) {
    case EffectiveStatus::Inactive: return QStringLiteral("inactive");
    case EffectiveStatus::Activating: return QStringLiteral("activating");
    case EffectiveStatus::Active: return QStringLiteral("active");
    case EffectiveStatus::PartiallyActive: return QStringLiteral("partially-active");
    case EffectiveStatus::Unsupported: return QStringLiteral("unsupported");
    case EffectiveStatus::PermissionDenied: return QStringLiteral("permission-denied");
    case EffectiveStatus::Failed: return QStringLiteral("failed");
    }
    return QString();
}

// Inhibition request record (spec §6.2)
struct InhibitionRequest {
    QUuid id;
    RequestSource source = RequestSource::Manual;
    Capabilities capabilities;
    QString reason;
    QDateTime createdAt;
    ExpirationPolicy expiration = ExpirationPolicy::None;
    qint64 durationMs = 0;      // when expiration == Duration
    QDateTime until;            // when expiration == UntilTime (absolute instant)
    int priority = 0;
};

// A backend binding: one acquired resource for one capability set
enum class BindingState : quint8 { Activating, Active, Failed, Denied, Lost };
inline QString bindingStateName(BindingState s)
{
    switch (s) {
    case BindingState::Activating: return QStringLiteral("activating");
    case BindingState::Active: return QStringLiteral("active");
    case BindingState::Failed: return QStringLiteral("failed");
    case BindingState::Denied: return QStringLiteral("denied");
    case BindingState::Lost: return QStringLiteral("lost");
    }
    return QString();
}

struct InhibitionBinding {
    QString id;
    Capabilities capabilities;
    QString backendId;
    BindingState state = BindingState::Activating;
    QString errorDetail;
    QDateTime acquiredAt;
};

// Effective Keep Awake state published to UI/CLI (requested ≠ effective)
struct EffectiveAwakeState {
    EffectiveStatus status = EffectiveStatus::Inactive;
    Capabilities requested;
    Capabilities effective;
    QString backend;            // backend serving effective bindings
    QString reason;             // highest-priority eligible reason
    qint64 remainingMs = -1;    // -1 = indefinite/none
    QString limitation;         // plain-language limitation text when partial
    QString errorDetail;
    QVariantMap toJson() const;
};

// Modules
enum class Module : quint8 { KeepAwake, Focus, WorkLog };
inline QString moduleName(Module m)
{
    switch (m) {
    case Module::KeepAwake: return QStringLiteral("keep-awake");
    case Module::Focus: return QStringLiteral("focus");
    case Module::WorkLog: return QStringLiteral("work-log");
    }
    return QString();
}

} // namespace kaeru
