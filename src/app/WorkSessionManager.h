// Kaeru — work session manager (spec §6.4, WP-05)
// Owns the single active work session: start/pause/resume/stop, project
// switching (transactional — the old segment closes before the new one opens),
// suspend/lock boundary handling (close the open segment; never invent work),
// and 60 s recovery checkpoints. All accounting is suspend-exclusive: paused
// and suspended time is recorded as separate 'paused' segments, never work.
#pragma once
#include "storage/SessionRepository.h"

#include <QDateTime>
#include <QObject>
#include <QString>

namespace kaeru {

class ProjectRepository;
class Settings;

class WorkSessionManager final : public QObject {
    Q_OBJECT
public:
    WorkSessionManager(SessionRepository* sessions, ProjectRepository* projects,
                       QObject* parent = nullptr);

    void setSettings(Settings* settings);

    // Recording commands (all return false with a non-empty error when not
    // applicable — never silently ignored).
    bool start(const QString& projectName, QString* error);
    bool pause(const QString& source, QString* error);  // source: manual|auto-lock|auto-suspend|quit
    bool resume(QString* error);
    bool stop(QString* error);                          // closes segment AND session
    // Project switch while recording: transactional close + reopen.
    bool switchProject(const QString& projectName, QString* error);
    // Focus interval recorded into focus_sessions (spec §6.4 separate table).
    // Work-log segment totals are NOT touched — the same minute is never
    // counted twice (no double counting, no back-dated segments).
    bool recordFocusWork(qint64 plannedMs, qint64 achievedMs, QString* error);

    bool isActive() const;
    // True when a session is active AND its newest segment is still open.
    bool isRecording() const;
    SessionRecord session() const;
    qint64 todayWorkMs() const;

    // Periodic tick (Application 1 Hz): writes a checkpoint every 60 s while
    // recording. Checkpoints are resume evidence only — never report data.
    void tick();
    // Suspend boundary (logind PrepareForSleep): close the open segment.
    void onSuspendBoundary(bool suspending);
    // Lock boundary: pause recording when the user setting says so.
    void onLockChanged(bool locked);

    // Recovery (startup): conservative — recover only up to the last reliable
    // checkpoint, mark the rest uncertain, never invent work.
    struct RecoveryOutcome {
        bool recoveredAny = false;
        QStringList notes; // user-facing, honest
    };
    RecoveryOutcome recoverInterrupted();

signals:
    void stateChanged();

private:
    QString ensureProjectId(const QString& projectName, QString* error);
    qint64 checkpointIntervalMs() const { return 60 * 1000; }

    SessionRepository* m_sessions = nullptr;
    ProjectRepository* m_projects = nullptr;
    Settings* m_settings = nullptr;
    qint64 m_lastCheckpointMs = 0;
};

} // namespace kaeru