// Kaeru — inhibition request registry (spec §6.2)
#pragma once
#include "Types.h"

#include <QList>
#include <QObject>

namespace kaeru {

class InhibitionRegistry final : public QObject {
    Q_OBJECT
public:
    explicit InhibitionRegistry(QObject* parent = nullptr);

    // Adds a request and returns its id.
    QUuid add(const InhibitionRequest& request);
    bool remove(const QUuid& id);          // explicit release of one request
    void clear();
    QList<InhibitionRequest> requests() const;
    bool contains(const QUuid& id) const;

    // Removes expired requests given wall time now; returns removed ids.
    QList<QUuid> expireElapsed(const QDateTime& nowUtc);

    // Remaining time until the earliest expiry among active requests; -1 if none.
    qint64 earliestRemainingMs(const QDateTime& nowUtc) const;

    // Highest-priority reason among eligible requests (empty when none).
    QString primaryReason() const;

signals:
    void changed();

private:
    QList<InhibitionRequest> m_requests;
};

} // namespace kaeru
