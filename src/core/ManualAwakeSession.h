// Kaeru — single manual Keep Awake session (initial release)
//
// The user-facing Keep Awake model deliberately holds at most ONE manual
// request at a time. Choosing a new duration replaces the previous request
// instead of stacking another one (the cause of "2h still shows 15m").
//
// This class is pure core state: it builds the InhibitionRequest and tracks
// the active request id. Application owns the coordinator/registry calls.
#pragma once
#include "Types.h"

#include <QDateTime>
#include <QUuid>

namespace kaeru {

class ManualAwakeSession final {
public:
    // mode: "system" | "display" | "both" (same vocabulary as CLI/UI).
    static InhibitionRequest timedRequest(qint64 durationMs, const QString& mode,
        const QString& reason = QString());
    static InhibitionRequest indefiniteRequest(const QString& mode,
        const QString& reason = QString());
    static InhibitionRequest untilRequest(const QDateTime& untilUtc, const QString& mode,
        const QString& reason = QString());

    // Stores the request as the one manual session. Returns the assigned id
    // (null when the request is not usable). Any previous session is replaced.
    QUuid start(const InhibitionRequest& request);

    // Forgets the session. Does not touch the coordinator/registry.
    void stop();

    bool isActive() const { return !m_request.id.isNull(); }
    QUuid requestId() const { return m_request.id; }
    InhibitionRequest request() const { return m_request; }
    bool isIndefinite() const
    {
        return isActive() && m_request.expiration == ExpirationPolicy::None;
    }

    // Remaining time for the current request; -1 when inactive or indefinite.
    qint64 remainingMs(const QDateTime& nowUtc) const;

private:
    InhibitionRequest m_request;
};

} // namespace kaeru
