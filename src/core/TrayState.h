// Kaeru — tray presentation state mapping (WP-03 frog identity, ADR-008)
// Pure function mapping the honest application state onto the three official
// frog expressions. Deterministic precedence, unit-tested; the tooltip text
// is assembled by the tray controller from the structured result.
//
// Expressions (official artwork, frog_icons/):
//   Awake   — keep-awake inhibition ACTIVELY held (never "activating")
//   Focused — a focus interval is actively running
//   Sleep   — Kaeru inactive (paused/all-off/failed) — NEVER implies the
//             computer is sleeping; the tooltip always states the app state.
//
// States can overlap (focus running while keep-awake is off, etc.). Three
// expressions cannot encode every combination; precedence (ADR-008):
//   1. Pause All / module off          → Sleep, summary "paused"/"off"
//   2. Keep-awake ACTIVE or PARTIAL    → Awake (safety-first; limitation in detail)
//   3. Focus running, no keep-awake    → Focused
//   4. Anything else                   → Sleep (error detail preserved)
// The tooltip carries every active fact, so no state is ever miscommunicated.
#pragma once
#include "Duration.h"
#include "Types.h"

#include <QString>

namespace kaeru {

struct TrayPresentation {
    enum class Icon : quint8 { Awake, Focused, Sleep };
    Icon icon = Icon::Sleep;
    bool keepAwakeActive = false;
    bool keepAwakePartial = false;
    bool focusActive = false;
    bool pausedAll = false;
    bool error = false;         // keep-awake requested but failed/denied
    QString summary;            // short status word for the tooltip
    QString detail;             // reason / limitation / error detail
};

// Compact tray-menu state (not the tooltip). The interactive status is the
// only activation control: it shows an honest ON/OFF line plus a second,
// quieter detail line. "ON" is never derived from a selected duration — the
// coordinator must have confirmed at least one active binding.
struct TrayMenuState {
    QString statusLine;
    QString detailLine;
    bool active = false;   // confirmed inhibition (ON)
    bool error = false;    // failed/denied/unsupported; never ON
};

inline TrayMenuState decideTrayMenuState(EffectiveStatus awakeStatus,
                                         qint64 remainingMs)
{
    TrayMenuState m;
    switch (awakeStatus) {
    case EffectiveStatus::Active:
    case EffectiveStatus::PartiallyActive:
        m.statusLine = QStringLiteral("Kaeru: ON");
        m.detailLine = remainingMs >= 0
            ? QStringLiteral("%1 remaining").arg(formatRemainingMs(remainingMs))
            : QStringLiteral("Indefinite");
        m.active = true;
        break;
    case EffectiveStatus::Activating:
        m.statusLine = QStringLiteral("Kaeru: ACTIVATING…");
        m.detailLine = QStringLiteral("Starting…");
        break;
    case EffectiveStatus::PermissionDenied:
    case EffectiveStatus::Failed:
    case EffectiveStatus::Unsupported:
        m.statusLine = QStringLiteral("Kaeru: ERROR");
        m.detailLine = QStringLiteral("Inactive");
        m.error = true;
        break;
    case EffectiveStatus::Inactive:
    default:
        m.statusLine = QStringLiteral("Kaeru: OFF");
        m.detailLine = QStringLiteral("Inactive");
        break;
    }
    return m;
}

inline TrayPresentation decideTrayPresentation(EffectiveStatus awakeStatus,
                                               bool focusActive, bool pausedAll,
                                               const QString& errorDetail,
                                               const QString& limitation)
{
    TrayPresentation p;
    p.focusActive = focusActive;
    p.pausedAll = pausedAll;

    if (pausedAll) {
        p.summary = QStringLiteral("paused (Pause All)");
        return p;
    }

    const bool awakeActive = awakeStatus == EffectiveStatus::Active
        || awakeStatus == EffectiveStatus::PartiallyActive;
    p.keepAwakeActive = awakeActive;
    p.keepAwakePartial = awakeStatus == EffectiveStatus::PartiallyActive;

    if (awakeActive) {
        // Safety-first: protection status wins the icon; focus (if any) is
        // stated in the tooltip, never hidden.
        p.icon = TrayPresentation::Icon::Awake;
        p.summary = p.keepAwakePartial ? QStringLiteral("keep awake (partial)")
                                       : QStringLiteral("keep awake active");
        if (!limitation.isEmpty())
            p.detail = limitation;
        return p;
    }

    if (focusActive) {
        p.icon = TrayPresentation::Icon::Focused;
        p.summary = QStringLiteral("focus running");
        return p;
    }

    // Neither active. Failed/denied keep-awake is an error the user asked
    // for — surface it honestly rather than showing a calm inactive face.
    if (awakeStatus == EffectiveStatus::Failed
        || awakeStatus == EffectiveStatus::PermissionDenied) {
        p.error = true;
        p.summary = awakeStatus == EffectiveStatus::PermissionDenied
            ? QStringLiteral("keep awake denied")
            : QStringLiteral("keep awake failed");
        p.detail = errorDetail;
    } else if (awakeStatus == EffectiveStatus::Activating) {
        p.summary = QStringLiteral("activating…");
    } else {
        p.summary = QStringLiteral("inactive");
    }
    return p;
}

} // namespace kaeru