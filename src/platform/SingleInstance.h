// Kaeru — single-instance + IPC via the session bus (spec §7, ADR-005)
// The service name io.github.kaeru.Kaeru is the instance lock. A second
// launch either forwards CLI commands or activates the running instance.
#pragma once
#include <QObject>
#include <QString>

#include <functional>

namespace kaeru {

class SingleInstance final : public QObject {
    Q_OBJECT
public:
    explicit SingleInstance(QObject* parent = nullptr);

    // Tries to own the service name. Returns true if this process is the
    // primary instance; false when another instance owns it.
    bool tryAcquire();

    // Second-launch helper: asks the running instance to show its window.
    // Returns false when no instance is running.
    bool activateExisting();

signals:
    void activationRequested();

private:
    bool m_primary = false;
};

} // namespace kaeru
