// Kaeru — CSV export (spec §12, KAE-REPORT-001)
// Spreadsheet formula-injection protection (OWASP CSV): any field whose
// first character could execute as a formula (=, +, -, @, tab, CR) is
// prefixed with an apostrophe and quoted like every other field.
#pragma once
#include <QDateTime>
#include <QList>
#include <QString>

namespace kaeru {

class SessionRepository;
class ProjectRepository;

class CsvExport final {
public:
    struct Row {
        qint64 sessionId = 0;
        qint64 segmentId = 0;
        QString project;
        QDateTime startUtc;
        QDateTime endUtc;   // invalid for still-open segments
        QString kind;
        QString source;
    };

    // Gathers closed (and open, marked) segments in the range with project names.
    static QList<Row> gather(SessionRepository& sessions, ProjectRepository& projects,
                             const QDateTime& fromUtc, const QDateTime& toUtc,
                             QString* error = nullptr);

    // Serializes rows; header always present. Returns empty string on nothing.
    static QString serialize(const QList<Row>& rows);

    // One-field escape/quoting including formula-injection guard.
    static QString escapeField(const QString& raw);

    // Writes UTF-8 CSV to destPath (no trailing newline after last row).
    static bool write(const QString& csv, const QString& destPath,
                      QString* error = nullptr, int* rowCount = nullptr);

    // Standard filename: kaeru-worklog-YYYYMMDD-YYYYMMDD.csv (uiux §6)
    static QString fileNameFor(const QDate& from, const QDate& to);
};

} // namespace kaeru
