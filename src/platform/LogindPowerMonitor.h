// Kaeru — logind power monitor: PrepareForSleep signal + CanSuspend state
// (spec §6.6: suspend boundary handling; also feeds diagnostics evidence)
#pragma once
#include <QObject>

namespace kaeru {

class LogindPowerMonitor final : public QObject {
    Q_OBJECT
public:
    explicit LogindPowerMonitor(QObject* parent = nullptr);
    bool available() const { return m_available; }
    bool preparingForSleep() const { return m_preparing; }

signals:
    void prepareForSleep(bool start);

private slots:
    void onPrepareForSleepSignal(bool start) { m_preparing = start; emit prepareForSleep(start); }

private:
    bool m_available = false;
    bool m_preparing = false;
};

} // namespace kaeru
