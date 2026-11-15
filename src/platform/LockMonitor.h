// Kaeru — screen lock monitor (spec §6.6)
// Sources: org.gnome.ScreenSaver ActiveChanged (GNOME) + logind LockedHint.
// Never unlocks; only observes (privacy: no lock circumvention).
#pragma once
#include <QObject>

namespace kaeru {

class LockMonitor final : public QObject {
    Q_OBJECT
public:
    explicit LockMonitor(QObject* parent = nullptr);

    bool available() const { return m_available; }
    QString sourceName() const { return m_source; }
    bool isLocked() const { return m_locked; }

signals:
    void lockStateChanged(bool locked);

private slots:
    void onGnomeActiveChanged(bool active);
    void onPropertiesChanged(const QString& interface, const QVariantMap& changed, const QStringList& invalidated);

private:
    bool m_available = false;
    bool m_locked = false;
    QString m_source;
};

} // namespace kaeru
