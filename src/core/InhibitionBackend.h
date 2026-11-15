// Kaeru — inhibition backend interface (spec §6.1, ADR-002)
// Core-side interface so coordinators are testable without DBus.
#pragma once
#include "Types.h"

#include <QObject>

namespace kaeru {

// One acquire() call returns a binding id immediately; the actual activation
// is asynchronous and signalled via bindingActivated / bindingFailed.
// A submitted request is NOT proof of active inhibition (spec §14).
class InhibitionBackend : public QObject {
    Q_OBJECT
public:
    explicit InhibitionBackend(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    ~InhibitionBackend() override = default;

    virtual QString backendId() const = 0;
    // Capabilities this backend can honestly provide on this system.
    virtual Capabilities supportedCapabilities() const = 0;
    // Human-readable availability detail for diagnostics ("ok", reasons).
    virtual QString availabilityDetail() const = 0;

    // Begin acquiring 'capabilities' as one binding. Implementations must
    // emit exactly one of bindingActivated/bindingFailed/bindingLost for the
    // returned binding id (eventually).
    virtual QString acquire(Capabilities capabilities, const QString& reason) = 0;
    // Release a binding; idempotent.
    virtual void release(const QString& bindingId) = 0;
    // True while the backend service is reachable (used by diagnostics).
    virtual bool serviceReachable() const = 0;

signals:
    void bindingActivated(const QString& bindingId);
    void bindingFailed(const QString& bindingId, bool permissionDenied, const QString& detail);
    void bindingLost(const QString& bindingId, const QString& reason); // restart/loss
};

} // namespace kaeru
