// Kaeru unit tests — CLI parsing (spec §12, ADR-005)
// Malformed values are rejected explicitly, never guessed.
#include "app/CliParser.h"

#include <QDate>
#include <QtTest>

using namespace kaeru;

class TestCliParser final : public QObject {
    Q_OBJECT

private slots:
    void noArgsMeansGui()
    {
        const CliInvocation inv = CliParser::parse({});
        QCOMPARE(inv.mode, CliInvocation::Mode::Gui);
        QVERIFY(!inv.isUsageError());
    }

    void helpAndVersion()
    {
        CliInvocation inv = CliParser::parse({ QStringLiteral("--help") });
        QCOMPARE(inv.mode, CliInvocation::Mode::Help);
        inv = CliParser::parse({ QStringLiteral("--version") });
        QCOMPARE(inv.mode, CliInvocation::Mode::Version);
        QVERIFY(!inv.isUsageError());
    }

    void backgroundFlag()
    {
        const CliInvocation inv = CliParser::parse({ QStringLiteral("--background") });
        QCOMPARE(inv.mode, CliInvocation::Mode::GuiBackground);
    }

    void statusCommand()
    {
        const CliInvocation inv = CliParser::parse({ QStringLiteral("status") });
        QCOMPARE(inv.mode, CliInvocation::Mode::Command);
        QCOMPARE(inv.command, QStringLiteral("status"));
    }

    void awakeWithDurationAndMode()
    {
        const CliInvocation inv = CliParser::parse({
            QStringLiteral("awake"), QStringLiteral("on"),
            QStringLiteral("--duration"), QStringLiteral("90s"),
            QStringLiteral("--mode"), QStringLiteral("system"),
        });
        QCOMPARE(inv.mode, CliInvocation::Mode::Command);
        QCOMPARE(inv.command, QStringLiteral("awake"));
        QCOMPARE(inv.operands, QStringList { QStringLiteral("on") });
        // Durations are parsed once, validated here (never re-guessed later).
        QCOMPARE(inv.options.value(QStringLiteral("durationMs")).toLongLong(), qint64(90000));
        QCOMPARE(inv.options.value(QStringLiteral("mode")).toString(),
                 QStringLiteral("system"));
    }

    void awakeIndefiniteAndOff()
    {
        CliInvocation inv = CliParser::parse({ QStringLiteral("awake"), QStringLiteral("off") });
        QCOMPARE(inv.operands, QStringList { QStringLiteral("off") });
        inv = CliParser::parse({ QStringLiteral("awake"), QStringLiteral("indefinite") });
        QCOMPARE(inv.operands, QStringList { QStringLiteral("indefinite") });
    }

    void exportWithDates()
    {
        const CliInvocation inv = CliParser::parse({
            QStringLiteral("export"), QStringLiteral("--format"), QStringLiteral("csv"),
            QStringLiteral("--from"), QStringLiteral("2026-11-01"),
            QStringLiteral("--to"), QStringLiteral("2026-11-30"),
        });
        QCOMPARE(inv.command, QStringLiteral("export"));
        QCOMPARE(inv.options.value(QStringLiteral("format")).toString(), QStringLiteral("csv"));
        QCOMPARE(inv.options.value(QStringLiteral("from")).toString(), QStringLiteral("2026-11-01"));
        QCOMPARE(inv.options.value(QStringLiteral("to")).toString(), QStringLiteral("2026-11-30"));
    }

    void malformedDurationRejected()
    {
        const CliInvocation inv = CliParser::parse({
            QStringLiteral("awake"), QStringLiteral("on"),
            QStringLiteral("--duration"), QStringLiteral("1x"),
        });
        QVERIFY(inv.isUsageError());
        QVERIFY(!inv.error.isEmpty());
    }

    void unknownModeRejected()
    {
        const CliInvocation inv = CliParser::parse({
            QStringLiteral("awake"), QStringLiteral("on"),
            QStringLiteral("--mode"), QStringLiteral("reboot"),
        });
        QVERIFY(inv.isUsageError());
    }

    void malformedDateRejected()
    {
        const CliInvocation inv = CliParser::parse({
            QStringLiteral("export"),
            QStringLiteral("--from"), QStringLiteral("31/12/2026"),
        });
        QVERIFY(inv.isUsageError());
    }

    void awakeWithoutOperandPassesThrough()
    {
        // The parser carries the command through; the runner prints usage and
        // exits with code 2 for missing verbs (integration-tested separately).
        const CliInvocation inv = CliParser::parse({ QStringLiteral("awake") });
        QCOMPARE(inv.mode, CliInvocation::Mode::Command);
        QCOMPARE(inv.command, QStringLiteral("awake"));
        QCOMPARE(inv.operands.isEmpty(), true);
        QVERIFY(!inv.isUsageError());
    }

    void unknownCommandRejected()
    {
        const CliInvocation inv = CliParser::parse({ QStringLiteral("frobnicate") });
        QVERIFY(inv.isUsageError());
    }
};

QTEST_MAIN(TestCliParser)
#include "test_cliparser.moc"
