// Kaeru — desktop notifications via org.freedesktop.Notifications (spec §6.3)
// Nonintrusive: standard notifications; no fullscreen interruption; never
// changes system Do Not Disturb. Quiet hours are enforced by callers.
#pragma once
#include <QObject>
#include <QString>

namespace kaeru {

class Notifications final : public QObject {
    Q_OBJECT
public:
    explicit Notifications(QObject* parent = nullptr);

    bool available() const;
    void notify(const QString& title, const QString& body, const QString& iconName = QString());

signals:
    void notificationClosed(uint id, uint reason);
    void notificationActioned(uint id, const QString& actionKey);

private:
    bool m_available = false;
};

} // namespace kaeru
