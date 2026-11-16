#include "ManualAwakeSession.h"

namespace kaeru {

static Capabilities capabilitiesForMode(const QString& mode)
{
    if (mode == QStringLiteral("system"))
        return Capability::SystemSleep;
    if (mode == QStringLiteral("display"))
        return Capability::SessionIdle;
    return Capability::SystemSleep | Capability::SessionIdle;
}

InhibitionRequest ManualAwakeSession::timedRequest(qint64 durationMs, const QString& mode,
    const QString& reason)
{
    InhibitionRequest r;
    r.source = RequestSource::Countdown;
    r.capabilities = capabilitiesForMode(mode);
    r.reason = reason.isEmpty() ? QStringLiteral("Keep awake (timed)") : reason;
    r.priority = 10;
    r.expiration = ExpirationPolicy::Duration;
    r.durationMs = durationMs;
    return r;
}

InhibitionRequest ManualAwakeSession::indefiniteRequest(const QString& mode,
    const QString& reason)
{
    InhibitionRequest r;
    r.source = RequestSource::Manual;
    r.capabilities = capabilitiesForMode(mode);
    r.reason = reason.isEmpty() ? QStringLiteral("Keep awake (indefinite)") : reason;
    r.priority = 10;
    r.expiration = ExpirationPolicy::None;
    return r;
}

InhibitionRequest ManualAwakeSession::untilRequest(const QDateTime& untilUtc,
    const QString& mode, const QString& reason)
{
    InhibitionRequest r;
    r.source = RequestSource::Manual;
    r.capabilities = capabilitiesForMode(mode);
    r.reason = reason.isEmpty()
        ? QStringLiteral("Keep awake (until %1)")
              .arg(untilUtc.toLocalTime().toString(QStringLiteral("HH:mm")))
        : reason;
    r.priority = 10;
    r.expiration = ExpirationPolicy::UntilTime;
    r.until = untilUtc;
    return r;
}

QUuid ManualAwakeSession::start(const InhibitionRequest& request)
{
    if (request.capabilities == 0)
        return QUuid();
    if (request.expiration == ExpirationPolicy::Duration && request.durationMs <= 0)
        return QUuid();
    if (request.expiration == ExpirationPolicy::UntilTime && !request.until.isValid())
        return QUuid();

    m_request = request;
    m_request.id = QUuid::createUuid();
    if (!m_request.createdAt.isValid())
        m_request.createdAt = QDateTime::currentDateTimeUtc();
    return m_request.id;
}

void ManualAwakeSession::stop()
{
    m_request = InhibitionRequest();
}

qint64 ManualAwakeSession::remainingMs(const QDateTime& nowUtc) const
{
    if (!isActive())
        return -1;
    QDateTime expiry;
    if (m_request.expiration == ExpirationPolicy::Duration)
        expiry = m_request.createdAt.addMSecs(m_request.durationMs);
    else if (m_request.expiration == ExpirationPolicy::UntilTime)
        expiry = m_request.until;
    else
        return -1;
    if (!expiry.isValid())
        return -1;
    return qMax<qint64>(0, nowUtc.msecsTo(expiry));
}

} // namespace kaeru
