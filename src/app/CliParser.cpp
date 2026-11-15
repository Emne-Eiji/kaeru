#include "CliParser.h"
#include "core/Duration.h"

namespace kaeru {

bool CliParser::parseDurationOption(const QString& value, qint64* ms)
{
    return parseDurationMs(value, ms);
}

bool CliParser::parseDateOption(const QString& value, QDate* d)
{
    return parseIsoDate(value, d);
}

bool CliParser::parseModeOption(const QString& value, QString* mode)
{
    const QString v = value.toLower();
    if (v == QStringLiteral("system") || v == QStringLiteral("display") || v == QStringLiteral("both")) {
        *mode = v;
        return true;
    }
    return false;
}

CliInvocation CliParser::parse(const QStringList& args)
{
    CliInvocation inv;

    if (args.isEmpty())
        return inv; // GUI mode

    const QString first = args.first();

    if (first == QStringLiteral("--help") || first == QStringLiteral("-h") || first == QStringLiteral("help")) {
        inv.mode = CliInvocation::Mode::Help;
        return inv;
    }
    if (first == QStringLiteral("--version")) {
        inv.mode = CliInvocation::Mode::Version;
        return inv;
    }
    if (first == QStringLiteral("--background")) {
        inv.mode = CliInvocation::Mode::GuiBackground;
        return inv;
    }

    static const QStringList knownCommands = {
        QStringLiteral("status"), QStringLiteral("awake"), QStringLiteral("focus"),
        QStringLiteral("work"), QStringLiteral("export"), QStringLiteral("pause-all"),
        QStringLiteral("diagnostics"), QStringLiteral("open"), QStringLiteral("quit")
    };

    if (!knownCommands.contains(first)) {
        inv.mode = CliInvocation::Mode::Command;
        inv.error = QStringLiteral("unknown command '%1' (try: status, awake, focus, work, export, pause-all, diagnostics, open, quit)").arg(first);
        return inv;
    }

    inv.mode = CliInvocation::Mode::Command;
    inv.command = first;

    for (int i = 1; i < args.size(); ++i) {
        const QString a = args.at(i);
        if (a == QStringLiteral("--help") || a == QStringLiteral("-h")) {
            inv.mode = CliInvocation::Mode::Help;
            inv.command = first;
            return inv;
        }
        if (a.startsWith(QStringLiteral("--"))) {
            const QString key = a.mid(2);
            if (key == QStringLiteral("json")) {
                inv.options[QStringLiteral("json")] = true;
                continue;
            }
            // Options that take a value
            const QString next = (i + 1 < args.size()) ? args.at(i + 1) : QString();
            if (key == QStringLiteral("duration")) {
                qint64 ms = 0;
                if (!parseDurationOption(next, &ms)) {
                    inv.error = QStringLiteral("invalid value for --duration: '%1' (use e.g. 30m, 1h30m, 90s)").arg(next);
                    return inv;
                }
                inv.options[QStringLiteral("durationMs")] = ms;
                ++i;
                continue;
            }
            if (key == QStringLiteral("mode")) {
                QString mode;
                if (!parseModeOption(next, &mode)) {
                    inv.error = QStringLiteral("invalid value for --mode: '%1' (system|display|both)").arg(next);
                    return inv;
                }
                inv.options[QStringLiteral("mode")] = mode;
                ++i;
                continue;
            }
            if (key == QStringLiteral("project")) {
                if (next.isEmpty()) {
                    inv.error = QStringLiteral("--project requires a value");
                    return inv;
                }
                inv.options[QStringLiteral("project")] = next;
                ++i;
                continue;
            }
            if (key == QStringLiteral("format")) {
                if (next.toLower() != QStringLiteral("csv")) {
                    inv.error = QStringLiteral("unsupported --format '%1' (only csv)").arg(next);
                    return inv;
                }
                inv.options[QStringLiteral("format")] = QStringLiteral("csv");
                ++i;
                continue;
            }
            if (key == QStringLiteral("from") || key == QStringLiteral("to")) {
                QDate d;
                if (!parseDateOption(next, &d)) {
                    inv.error = QStringLiteral("invalid date for --%1: '%2' (use YYYY-MM-DD)").arg(key, next);
                    return inv;
                }
                inv.options[key] = d.toString(QStringLiteral("yyyy-MM-dd"));
                ++i;
                continue;
            }
            if (key == QStringLiteral("output")) {
                inv.options[QStringLiteral("output")] = next;
                ++i;
                continue;
            }
            inv.error = QStringLiteral("unknown option '--%1'").arg(key);
            return inv;
        }
        inv.operands.append(a);
    }

    return inv;
}

} // namespace kaeru
