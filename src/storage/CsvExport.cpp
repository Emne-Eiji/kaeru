// Kaeru — CSV export (spec §12)
#include "CsvExport.h"
#include "ProjectRepository.h"
#include "SessionRepository.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTextStream>

namespace kaeru {

QString CsvExport::escapeField(const QString& raw)
{
    QString value = raw;
    // Formula-injection guard: neutralize leading formula trigger characters.
    static const QString triggers = QStringLiteral("=+-@\t\r");
    if (!value.isEmpty() && triggers.contains(value.at(0)))
        value = QLatin1Char('\'') + value;

    bool mustQuote = value.contains(QLatin1Char(',')) || value.contains(QLatin1Char('"'))
        || value.contains(QLatin1Char('\n')) || value.contains(QLatin1Char('\r'));
    if (mustQuote) {
        QString escaped = value;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return value;
}

QList<CsvExport::Row> CsvExport::gather(SessionRepository& sessions, ProjectRepository& projects,
                                        const QDateTime& fromUtc, const QDateTime& toUtc,
                                        QString* error)
{
    QList<Row> out;
    const QList<SegmentRecord> segments =
        sessions.segmentsBetween(fromUtc, toUtc, QString(), 0, error);
    if (!error || error->isEmpty()) {
        QHash<qint64, QString> projectNames;
        for (const SegmentRecord& seg : segments) {
            Row r;
            r.sessionId = seg.sessionId;
            r.segmentId = seg.id;
            r.startUtc = seg.startedUtc;
            r.endUtc = seg.endedUtc;
            r.kind = seg.kind;
            r.source = seg.source;
            // Resolve the project name once per session (never raw ids in CSV).
            if (!projectNames.contains(seg.sessionId)) {
                SessionRecord s;
                ProjectRecord p;
                if (sessions.sessionById(seg.sessionId, &s) && projects.byId(s.projectId, &p))
                    projectNames.insert(seg.sessionId, p.name);
            }
            r.project = projectNames.value(seg.sessionId);
            out.append(r);
        }
    }
    return out;
}

QString CsvExport::serialize(const QList<Row>& rows)
{
    QString csv;
    csv += QStringLiteral("session_id,segment_id,project,start_utc,end_utc,duration_minutes,kind,source\n");
    for (const Row& r : rows) {
        QStringList fields;
        fields << QString::number(r.sessionId);
        fields << QString::number(r.segmentId);
        fields << escapeField(r.project);
        fields << r.startUtc.toString(Qt::ISODate);
        fields << (r.endUtc.isValid() ? r.endUtc.toString(Qt::ISODate) : QStringLiteral("open"));
        if (r.endUtc.isValid() && r.startUtc.isValid()) {
            const qint64 ms = r.startUtc.msecsTo(r.endUtc);
            fields << QString::number(ms > 0 ? ms / 60000 : 0);
        } else {
            fields << QString();
        }
        fields << escapeField(r.kind);
        fields << escapeField(r.source);
        csv += fields.join(QLatin1Char(',')) + QLatin1Char('\n');
    }
    return csv;
}

bool CsvExport::write(const QString& csv, const QString& destPath, QString* error, int* rowCount)
{
    if (rowCount)
        *rowCount = 0;
    QDir().mkpath(QFileInfo(destPath).absolutePath());
    QFile f(destPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = QStringLiteral("cannot write %1: %2").arg(destPath, f.errorString());
        return false;
    }
    const QByteArray data = csv.toUtf8();
    if (f.write(data) != data.size()) {
        if (error)
            *error = QStringLiteral("short write to %1: %2").arg(destPath, f.errorString());
        return false;
    }
    if (rowCount) {
        // Rows = lines minus header; ignore a trailing empty line.
        *rowCount = qMax(0, csv.count(QLatin1Char('\n')) - 1);
    }
    return true;
}

QString CsvExport::fileNameFor(const QDate& from, const QDate& to)
{
    return QStringLiteral("kaeru-worklog-%1-%2.csv")
        .arg(from.toString(QStringLiteral("yyyyMMdd")),
             to.toString(QStringLiteral("yyyyMMdd")));
}

} // namespace kaeru
