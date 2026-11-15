// Kaeru — work session / segment repository (spec §10, WP-05 storage core)
// Constraints enforced here (not just in schema): at most one open session,
// nonnegative durations, no overlapping segments within a session
// (append/close are transactional), recovery checkpoints never duplicate
// report entries (totals ignore checkpoints; they are only for resume).
#pragma once
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QSqlDatabase>
#include <QString>

namespace kaeru {

struct SessionRecord {
    qint64 id = 0;
    qint64 projectId = 0;
    QDateTime startedUtc;
    QDateTime endedUtc;   // invalid while open
    bool open = false;
    QString createdVia;
};

struct SegmentRecord {
    qint64 id = 0;
    qint64 sessionId = 0;
    QDateTime startedUtc;
    QDateTime endedUtc;   // invalid while open
    QString kind;         // work | break | paused | idle
    QString source;       // manual | focus | auto (suspend/lock boundary)
};

class SessionRepository final {
public:
    explicit SessionRepository(QSqlDatabase db);

    // --- sessions ---
    bool activeSession(SessionRecord* out, QString* error = nullptr) const;
    bool sessionById(qint64 id, SessionRecord* out, QString* error = nullptr) const;
    qint64 startSession(qint64 projectId, const QDateTime& atUtc, QString* error = nullptr);
    bool closeSession(qint64 sessionId, const QDateTime& atUtc, QString* error = nullptr);
    bool closeActiveSession(const QDateTime& atUtc, qint64* closedId, QString* error = nullptr);

    // --- segments ---
    // Opens a segment and closes any still-open segment of the same session
    // in one transaction (prevents overlap). 'atUtc' is the segment start.
    qint64 appendSegment(qint64 sessionId, const QDateTime& startUtc,
                         const QString& kind, const QString& source,
                         QString* error = nullptr);
    // Closes the newest open segment; returns its id (0 if none).
    qint64 closeOpenSegment(qint64 sessionId, const QDateTime& atUtc, QString* error = nullptr);
    QList<SegmentRecord> segmentsForSession(qint64 sessionId, QString* error = nullptr) const;
    QList<SegmentRecord> segmentsBetween(const QDateTime& fromUtc, const QDateTime& toUtc,
                                         const QString& kind, qint64 projectId,
                                         QString* error = nullptr) const;

    // --- totals (closed segments only; open segments never count as work) ---
    struct Totals {
        qint64 workMs = 0;
        qint64 breakMs = 0;
        int segmentCount = 0;
    };
    bool totalsBetween(const QDateTime& fromUtc, const QDateTime& toUtc,
                       qint64 projectId, Totals* out, QString* error = nullptr) const;

    struct DailyTotal {
        QDate day;   // local day derived from UTC instant + timezone offset
        qint64 workMs = 0;
        qint64 breakMs = 0;
    };
    QList<DailyTotal> dailyTotals(const QDateTime& fromUtc, const QDateTime& toUtc,
                                  int utcOffsetSeconds, QString* error = nullptr) const;

    // --- focus sessions (spec: separate table) ---
    bool insertFocusSession(const QDateTime& startUtc, const QDateTime& endUtc,
                            qint64 plannedMs, qint64 achievedMs,
                            const QString& phase, const QString& mode,
                            QString* error = nullptr);

    // --- recovery checkpoints (resume evidence only, never report data) ---
    bool writeCheckpoint(qint64 sessionId, qint64 segmentId, const QDateTime& atUtc,
                         const QString& kind, QString* error = nullptr);
    bool deleteCheckpointsForSession(qint64 sessionId, QString* error = nullptr);

private:
    QSqlDatabase m_db;
    static QString iso(const QDateTime& utc);
};

} // namespace kaeru
