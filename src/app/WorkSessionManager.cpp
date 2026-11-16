// Kaeru — work session manager (spec §6.4, WP-05)
#include "WorkSessionManager.h"
#include "Settings.h"
#include "storage/ProjectRepository.h"
#include "storage/SessionRepository.h"

#include <QDateTime>
#include <QSqlQuery>

namespace kaeru {

WorkSessionManager::WorkSessionManager(SessionRepository* sessions, ProjectRepository* projects,
                                       QObject* parent)
    : QObject(parent)
    , m_sessions(sessions)
    , m_projects(projects)
{
}

void WorkSessionManager::setSettings(Settings* settings)
{
    m_settings = settings;
}

QString WorkSessionManager::ensureProjectId(const QString& projectName, QString* error)
{
    if (!m_projects)
        return QString();
    ProjectRecord p;
    if (m_projects->byName(projectName, &p, error) && p.id > 0)
        return QString::number(p.id);
    const QString name = projectName.trimmed().isEmpty()
        ? (m_settings ? m_settings->defaultProject() : QStringLiteral("General"))
        : projectName.trimmed();
    ProjectRecord existing;
    if (m_projects->byName(name, &existing, error) && existing.id > 0)
        return QString::number(existing.id);
    const qint64 id = m_projects->add(name, QString(), error);
    return id > 0 ? QString::number(id) : QString();
}

bool WorkSessionManager::start(const QString& projectName, QString* error)
{
    SessionRecord existing;
    if (m_sessions->activeSession(&existing, error)) {
        // Already recording: treat as resume of the open segment.
        return resume(error);
    }
    if (error)
        error->clear();
    const QString pidStr = ensureProjectId(projectName, error);
    if (pidStr.isEmpty())
        return false;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const qint64 sid = m_sessions->startSession(pidStr.toLongLong(), now, error);
    if (sid <= 0)
        return false;
    if (m_sessions->appendSegment(sid, now, QStringLiteral("work"), QStringLiteral("manual"), error) <= 0)
        return false;
    m_lastCheckpointMs = now.toMSecsSinceEpoch();
    emit stateChanged();
    return true;
}

bool WorkSessionManager::pause(const QString& source, QString* error)
{
    Q_UNUSED(source) // the work segment is closed; paused duration is implicit
    SessionRecord s;
    if (!m_sessions->activeSession(&s, error))
        return false;
    if (m_sessions->closeOpenSegment(s.id, QDateTime::currentDateTimeUtc(), error) <= 0) {
        if (error && error->isEmpty())
            *error = QStringLiteral("nothing to pause");
        return false;
    }
    emit stateChanged();
    return true;
}

bool WorkSessionManager::resume(QString* error)
{
    SessionRecord s;
    if (!m_sessions->activeSession(&s, error))
        return false;
    if (m_sessions->appendSegment(s.id, QDateTime::currentDateTimeUtc(),
                                  QStringLiteral("work"), QStringLiteral("manual"), error) <= 0)
        return false;
    m_lastCheckpointMs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    emit stateChanged();
    return true;
}

bool WorkSessionManager::stop(QString* error)
{
    SessionRecord s;
    if (!m_sessions->activeSession(&s, error))
        return false;
    return m_sessions->closeSession(s.id, QDateTime::currentDateTimeUtc(), error);
}

bool WorkSessionManager::switchProject(const QString& projectName, QString* error)
{
    SessionRecord s;
    if (!m_sessions->activeSession(&s, error))
        return false;
    const QString pidStr = ensureProjectId(projectName, error);
    if (pidStr.isEmpty())
        return false;
    // Transactional: close the open segment, then open the new one.
    if (m_sessions->closeOpenSegment(s.id, QDateTime::currentDateTimeUtc(), error) <= 0)
        return false;
    if (m_sessions->appendSegment(s.id, QDateTime::currentDateTimeUtc(),
                                  QStringLiteral("work"), QStringLiteral("project-switch"), error) <= 0)
        return false;
    emit stateChanged();
    return true;
}

bool WorkSessionManager::recordFocusWork(qint64 plannedMs, qint64 achievedMs, QString* error)
{
    if (plannedMs < 0 || achievedMs < 0) {
        if (error)
            *error = QStringLiteral("invalid focus session values");
        return false;
    }
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QDateTime start = achievedMs > 0 ? now.addMSecs(-achievedMs) : now;
    if (!m_sessions->insertFocusSession(start, now, plannedMs, achievedMs,
                                        QStringLiteral("work"), QStringLiteral("focus"), error))
        return false;
    return true;
}

bool WorkSessionManager::isActive() const
{
    SessionRecord s;
    return m_sessions->activeSession(&s, nullptr);
}

bool WorkSessionManager::isRecording() const
{
    SessionRecord s;
    if (!m_sessions->activeSession(&s, nullptr))
        return false;
    const QList<SegmentRecord> segs = m_sessions->segmentsForSession(s.id, nullptr);
    return !segs.isEmpty() && !segs.last().endedUtc.isValid();
}

SessionRecord WorkSessionManager::session() const
{
    SessionRecord s;
    m_sessions->activeSession(&s, nullptr);
    return s;
}

qint64 WorkSessionManager::todayWorkMs() const
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QDateTime midnightUtc = QDateTime(now.date(), QTime(0, 0), Qt::UTC);
    SessionRepository::Totals totals;
    if (!m_sessions->totalsBetween(midnightUtc, now, 0, &totals, nullptr))
        return 0;
    return totals.workMs;
}

void WorkSessionManager::tick()
{
    SessionRecord s;
    if (!m_sessions->activeSession(&s, nullptr))
        return;
    const qint64 nowMs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (nowMs - m_lastCheckpointMs < checkpointIntervalMs())
        return;
    // Checkpoint the newest open segment (resume evidence only).
    const QList<SegmentRecord> segs = m_sessions->segmentsForSession(s.id, nullptr);
    qint64 openSeg = 0;
    for (const SegmentRecord& seg : segs)
        if (!seg.endedUtc.isValid())
            openSeg = seg.id;
    if (openSeg > 0)
        m_sessions->writeCheckpoint(s.id, openSeg, QDateTime::currentDateTimeUtc(),
                                    QStringLiteral("60s"), nullptr);
    m_lastCheckpointMs = nowMs;
}

void WorkSessionManager::onSuspendBoundary(bool suspending)
{
    // Spec §6.4: close the open segment at the suspend boundary. On resume
    // the session stays paused — the user resumes explicitly (never invent).
    if (!suspending)
        return;
    SessionRecord s;
    if (!m_sessions->activeSession(&s, nullptr))
        return;
    m_sessions->closeOpenSegment(s.id, QDateTime::currentDateTimeUtc(), nullptr);
    emit stateChanged();
}

void WorkSessionManager::onLockChanged(bool locked)
{
    if (!locked || !m_settings || !m_settings->pauseWorkWhenLocked())
        return;
    SessionRecord s;
    if (!m_sessions->activeSession(&s, nullptr))
        return;
    m_sessions->closeOpenSegment(s.id, QDateTime::currentDateTimeUtc(), nullptr);
    emit stateChanged();
}

WorkSessionManager::RecoveryOutcome WorkSessionManager::recoverInterrupted()
{
    RecoveryOutcome out;
    SessionRecord s;
    if (!m_sessions->activeSession(&s, nullptr)) {
        out.notes << QStringLiteral("No interrupted session found.");
        return out;
    }
    // Conservative: recover only up to the last reliable checkpoint; the
    // remainder is uncertain. If no checkpoint exists, close the segment at
    // its own start (zero reliable work) — never invent time.
    const QList<SegmentRecord> segs = m_sessions->segmentsForSession(s.id, nullptr);
    SegmentRecord openSeg;
    for (const SegmentRecord& seg : segs)
        if (!seg.endedUtc.isValid())
            openSeg = seg;
    QDateTime recoverTo = openSeg.startedUtc;
    bool hadCheckpoint = false;
    QSqlQuery q(m_sessions->handle());
    q.prepare(QStringLiteral("SELECT MAX(recorded_at) FROM recovery_checkpoints WHERE session_id = :sid"));
    q.bindValue(QStringLiteral(":sid"), s.id);
    if (q.exec() && q.next()) {
        const QString iso = q.value(0).toString();
        if (!iso.isEmpty()) {
            recoverTo = QDateTime::fromString(iso, Qt::ISODate).toUTC();
            hadCheckpoint = true;
        }
    }
    m_sessions->closeOpenSegment(s.id, recoverTo, nullptr);
    m_sessions->closeSession(s.id, recoverTo, nullptr);
    m_sessions->deleteCheckpointsForSession(s.id, nullptr);
    out.recoveredAny = true;
    if (hadCheckpoint) {
        out.notes << tr("Recovered interrupted session #%1 up to the last checkpoint (%2). "
                        "Time after the checkpoint is uncertain and was not recorded.")
                         .arg(s.id).arg(recoverTo.toLocalTime().toString(QStringLiteral("HH:mm")));
    } else {
        out.notes << tr("Recovered interrupted session #%1. No checkpoint was found, so only "
                        "time up to the segment start (%2) was recorded — no work time was invented.")
                         .arg(s.id).arg(recoverTo.toLocalTime().toString(QStringLiteral("HH:mm")));
    }
    emit stateChanged();
    return out;
}

} // namespace kaeru