// Kaeru — notification policy (spec §6.3)
// Pure decision functions: deterministic, unit-testable, and never touch the
// system Do Not Disturb (quiet hours are enforced by *not sending*, ADR-002
// non-interference rule).
#pragma once
#include <QString>

namespace kaeru {

struct NotifyDecision {
    bool notify = false;
    QString reason; // why not (tests/diagnostics); empty when notifying
};

// Focus interval finished: honor the user's notification option and the
// quiet-hours window (never changes system DND).
inline NotifyDecision decideFocusIntervalNotification(bool notifyOnInterval, bool quietActive)
{
    NotifyDecision d;
    if (!notifyOnInterval) {
        d.reason = QStringLiteral("focus notifications disabled");
        return d;
    }
    if (quietActive) {
        d.reason = QStringLiteral("quiet hours active");
        return d;
    }
    d.notify = true;
    return d;
}

// Timed keep-awake request expired: user setting gate.
inline NotifyDecision decideKeepAwakeExpiryNotification(bool notifyEnabled)
{
    NotifyDecision d;
    if (!notifyEnabled) {
        d.reason = QStringLiteral("keep-awake notifications disabled");
        return d;
    }
    d.notify = true;
    return d;
}

} // namespace kaeru