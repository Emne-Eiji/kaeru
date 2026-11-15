// Kaeru — XDG autostart backend (spec §7)
// Opt-in, session-scoped, edits ONLY Kaeru's own user autostart entry.
// Never enables systemd lingering; never touches other apps' entries.
#pragma once
#include <QObject>
#include <QString>

namespace kaeru {

class AutostartBackend final : public QObject {
    Q_OBJECT
public:
    explicit AutostartBackend(QObject* parent = nullptr);

    bool isEnabled() const;
    bool enable(const QString& extraArgs = QString()); // default: --background
    bool disable();

    // Diagnostics: path of the entry and whether the desktop dir is usable.
    QString entryPath() const;
    QString lastError() const { return m_lastError; }

private:
    QString autostartDir() const;
    QString m_lastError;
};

} // namespace kaeru
