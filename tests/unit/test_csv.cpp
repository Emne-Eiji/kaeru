// Kaeru unit tests — CSV export + formula-injection protection (spec §12)
#include "storage/CsvExport.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace kaeru;

class TestCsv final : public QObject {
    Q_OBJECT

private slots:
    void plainFieldsUntouched()
    {
        QCOMPARE(CsvExport::escapeField(QStringLiteral("work")), QStringLiteral("work"));
        QCOMPARE(CsvExport::escapeField(QStringLiteral("Web site")), QStringLiteral("Web site"));
        QCOMPARE(CsvExport::escapeField(QString()), QString());
    }

    void specialCharactersQuoted()
    {
        QCOMPARE(CsvExport::escapeField(QStringLiteral("a,b")),
                 QStringLiteral("\"a,b\""));
        QCOMPARE(CsvExport::escapeField(QStringLiteral("say \"hi\"")),
                 QStringLiteral("\"say \"\"hi\"\"\""));
        QCOMPARE(CsvExport::escapeField(QStringLiteral("line1\nline2")),
                 QStringLiteral("\"line1\nline2\""));
    }

    void formulaInjectionNeutralized()
    {
        // OWASP CSV: leading = + - @ (and tab/CR) get an apostrophe prefix.
        QCOMPARE(CsvExport::escapeField(QStringLiteral("=SUM(A1)")),
                 QStringLiteral("'=SUM(A1)"));
        QCOMPARE(CsvExport::escapeField(QStringLiteral("+cmd")), QStringLiteral("'+cmd"));
        QCOMPARE(CsvExport::escapeField(QStringLiteral("-1")), QStringLiteral("'-1"));
        QCOMPARE(CsvExport::escapeField(QStringLiteral("@cmd")), QStringLiteral("'@cmd"));
        QCOMPARE(CsvExport::escapeField(QStringLiteral("\tstep")), QStringLiteral("'\tstep"));
        // A "-" in the middle is fine — only the first character matters.
        QCOMPARE(CsvExport::escapeField(QStringLiteral("work-log")), QStringLiteral("work-log"));
    }

    void serializeWithHeaderAndRanges()
    {
        QList<CsvExport::Row> rows;
        CsvExport::Row r;
        r.sessionId = 7;
        r.segmentId = 12;
        r.project = QStringLiteral("=shell()");
        r.startUtc = QDateTime(QDate(2026, 11, 15), QTime(9, 0), Qt::UTC);
        r.endUtc = QDateTime(QDate(2026, 11, 15), QTime(9, 30), Qt::UTC);
        r.kind = QStringLiteral("work");
        r.source = QStringLiteral("manual");
        rows.append(r);

        CsvExport::Row open = r;
        open.segmentId = 13;
        open.endUtc = QDateTime();
        rows.append(open);

        const QString csv = CsvExport::serialize(rows);
        const QStringList lines = csv.trimmed().split(QLatin1Char('\n'));
        QCOMPARE(lines.size(), 3);
        QCOMPARE(lines.at(0),
                 QStringLiteral("session_id,segment_id,project,start_utc,end_utc,duration_minutes,kind,source"));
        QCOMPARE(lines.at(1),
                 QStringLiteral("7,12,'=shell(),2026-11-15T09:00:00Z,2026-11-15T09:30:00Z,30,work,manual"));
        // Open segments are exported honestly with no duration.
        QVERIFY(lines.at(2).endsWith(QStringLiteral(",open,,work,manual")));
    }

    void roundTripThroughParser()
    {
        QList<CsvExport::Row> rows;
        CsvExport::Row r;
        r.sessionId = 1;
        r.segmentId = 2;
        r.project = QStringLiteral("R&D, \"phase 2\"");
        r.startUtc = QDateTime(QDate(2026, 11, 15), QTime(10, 0), Qt::UTC);
        r.endUtc = QDateTime(QDate(2026, 11, 15), QTime(10, 5), Qt::UTC);
        r.kind = QStringLiteral("work");
        r.source = QStringLiteral("focus");
        rows.append(r);

        const QString csv = CsvExport::serialize(rows);
        const QStringList lines = csv.trimmed().split(QLatin1Char('\n'));
        QCOMPARE(lines.size(), 2);
        // A naive split on commas is wrong for quoted fields; a real parser
        // would split correctly. We verify quoting by rejoining heuristically.
        QVERIFY(lines.at(1).contains(QStringLiteral("\"R&D, \"\"phase 2\"\"\"")));
        QCOMPARE(lines.at(1).section(QLatin1Char(','), 0, 1), QStringLiteral("1,2"));
        // Naive comma indices shift because the quoted project field holds a comma:
        QCOMPARE(lines.at(1).section(QLatin1Char(','), 4, 4), QStringLiteral("2026-11-15T10:00:00Z"));
        QCOMPARE(lines.at(1).section(QLatin1Char(','), 5, 5), QStringLiteral("2026-11-15T10:05:00Z"));
        QCOMPARE(lines.at(1).section(QLatin1Char(','), 6, 6), QStringLiteral("5"));
    }

    void writeCreatesFileAndCountsRows()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.path() + QStringLiteral("/nested/kaeru-worklog-20261115-20261115.csv");

        QList<CsvExport::Row> rows;
        CsvExport::Row r;
        r.sessionId = 3;
        r.segmentId = 4;
        r.project = QStringLiteral("Reports");
        r.startUtc = QDateTime(QDate(2026, 11, 15), QTime(11, 0), Qt::UTC);
        r.endUtc = QDateTime(QDate(2026, 11, 15), QTime(11, 45), Qt::UTC);
        r.kind = QStringLiteral("work");
        r.source = QStringLiteral("manual");
        rows.append(r);

        QString err;
        int count = -1;
        QVERIFY2(CsvExport::write(CsvExport::serialize(rows), path, &err, &count),
                 qPrintable(err));
        QCOMPARE(count, 1);
        QVERIFY(QFile::exists(path));

        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QString content = QString::fromUtf8(f.readAll());
        QVERIFY(content.startsWith(QStringLiteral("session_id,")));

        // Overwrite (truncate) semantics with zero rows → header only.
        QVERIFY(CsvExport::write(CsvExport::serialize({}), path, &err, &count));
        QCOMPARE(count, 0);
        QFile f2(path);
        QVERIFY(f2.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(f2.readAll()).split(QLatin1Char('\n')).size(), 2); // header + \n
    }

    void fileNameConvention()
    {
        QCOMPARE(CsvExport::fileNameFor(QDate(2026, 11, 1), QDate(2026, 11, 30)),
                 QStringLiteral("kaeru-worklog-20261101-20261130.csv"));
    }
};

QTEST_MAIN(TestCsv)
#include "test_csv.moc"
