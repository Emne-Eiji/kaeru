#include "InhibitionRegistry.h"

namespace kaeru {

InhibitionRegistry::InhibitionRegistry(QObject* parent)
    : QObject(parent)
{
}

QUuid InhibitionRegistry::add(const InhibitionRequest& request)
{
    InhibitionRequest r = request;
    if (r.id.isNull())
        r.id = QUuid::createUuid();
    if (!r.createdAt.isValid())
        r.createdAt = QDateTime::currentDateTimeUtc();
    m_requests.append(r);
    emit changed();
    return r.id;
}

bool InhibitionRegistry::remove(const QUuid& id)
{
    for (int i = 0; i < m_requests.size(); ++i) {
        if (m_requests.at(i).id == id) {
            m_requests.removeAt(i);
            emit changed();
            return true;
        }
    }
    return false;
}

void InhibitionRegistry::clear()
{
    if (m_requests.isEmpty())
        return;
    m_requests.clear();
    emit changed();
}

QList<InhibitionRequest> InhibitionRegistry::requests() const
{
    return m_requests;
}

bool InhibitionRegistry::contains(const QUuid& id) const
{
    for (const InhibitionRequest& r : m_requests)
        if (r.id == id)
            return true;
    return false;
}

QList<QUuid> InhibitionRegistry::expireElapsed(const QDateTime& nowUtc)
{
    QList<QUuid> removed;
    for (int i = m_requests.size() - 1; i >= 0; --i) {
        const InhibitionRequest& r = m_requests.at(i);
        bool expired = false;
        if (r.expiration == ExpirationPolicy::Duration) {
            expired = r.createdAt.addMSecs(r.durationMs) <= nowUtc;
        } else if (r.expiration == ExpirationPolicy::UntilTime) {
            expired = r.until.isValid() && r.until <= nowUtc;
        }
        if (expired) {
            removed.append(r.id);
            m_requests.removeAt(i);
        }
    }
    if (!removed.isEmpty())
        emit changed();
    return removed;
}

qint64 InhibitionRegistry::earliestRemainingMs(const QDateTime& nowUtc) const
{
    qint64 best = -1;
    for (const InhibitionRequest& r : m_requests) {
        QDateTime expiry;
        if (r.expiration == ExpirationPolicy::Duration)
            expiry = r.createdAt.addMSecs(r.durationMs);
        else if (r.expiration == ExpirationPolicy::UntilTime)
            expiry = r.until;
        else
            continue;
        if (!expiry.isValid())
            continue;
        const qint64 remaining = nowUtc.msecsTo(expiry);
        if (remaining < 0)
            continue;
        if (best < 0 || remaining < best)
            best = remaining;
    }
    return best;
}

QString InhibitionRegistry::primaryReason() const
{
    const InhibitionRequest* best = nullptr;
    for (const InhibitionRequest& r : m_requests) {
        if (!best || r.priority > best->priority)
            best = &r;
    }
    return best ? best->reason : QString();
}

} // namespace kaeru
