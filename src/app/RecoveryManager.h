// Kaeru — recovery manager (spec §6.6, WP-05 wiring)
// Detects interrupted sessions/segments on startup, builds a review report,
// and performs conservative recovery: recover only up to the last reliable
// checkpoint; mark the remainder uncertain; never invent work.
#pragma once
#include <QVariantList>
#include <QVariantMap>

namespace kaeru {

class RecoveryManager final {
public:
    struct Report {
        bool hasOpenSession = false;
        bool hasUnrecoveredGap = false;
        QVariantList sessions;   // per-session recovery facts
    };

    // Minimal interface for WP-02; full behavior implemented with storage.
    static Report emptyReport() { return Report{}; }
};

} // namespace kaeru
