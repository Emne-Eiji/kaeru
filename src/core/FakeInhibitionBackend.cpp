#include "FakeInhibitionBackend.h"

namespace kaeru {

FakeInhibitionBackend::FakeInhibitionBackend(QObject* parent)
    : InhibitionBackend(parent)
{
}

QString FakeInhibitionBackend::acquire(Capabilities caps, const QString& reason)
{
    Q_UNUSED(reason)
    Q_UNUSED(caps)
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (m_failCounter > 0) {
        --m_failCounter;
        const bool denied = m_failDenied;
        const QString detail = m_failDetail;
        QMetaObject::invokeMethod(this, [this, id, denied, detail] {
            emit bindingFailed(id, denied, detail);
        }, Qt::QueuedConnection);
        return id;
    }
    m_bindings.append(id);
    QMetaObject::invokeMethod(this, [this, id] {
        emit bindingActivated(id);
    }, Qt::QueuedConnection);
    return id;
}

void FakeInhibitionBackend::release(const QString& bindingId)
{
    m_bindings.removeAll(bindingId);
}

void FakeInhibitionBackend::failNext(bool permissionDenied, const QString& detail)
{
    m_failCounter = 1;
    m_failDenied = permissionDenied;
    m_failDetail = detail;
}

void FakeInhibitionBackend::simulateLost(const QString& bindingId, const QString& reason)
{
    m_bindings.removeAll(bindingId);
    emit bindingLost(bindingId, reason);
}

void FakeInhibitionBackend::simulateActivated(const QString& bindingId)
{
    emit bindingActivated(bindingId);
}

} // namespace kaeru
