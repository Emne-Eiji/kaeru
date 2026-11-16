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

    // Global startup flags may be combined (autostart uses
    // "--background --awake-on-login"). They are accepted only as a leading
    // block, never mixed with a command.
    int index = 0;
    bool sawGlobal = false;
    while (index < args.size()) {
        const QString a = args.at(index);
        if (a == QStringLiteral("--background")) {
            inv.mode = CliInvocation::Mode::GuiBackground;
            sawGlobal = true;
            ++index;
            continue;
        }
        if (a == QStringLiteral("--awake-on-login")) {
            inv.awakeOnLogin = true;
            inv.mode = CliInvocation::Mode::GuiBackground;
            sawGlobal = true;
            ++index;
            continue;
        }
        if (a == QStringLiteral("--help") || a == QStringLiteral("-h")) {
            inv.mode = CliInvocation::Mode::Help;
            return inv;
        }
        if (a == QStringLiteral("--version")) {
            inv.mode = CliInvocation::Mode::Version;
            return inv;
        }
        break;
    }
    if (sawGlobal && index < args.size()) {
        inv.error = QStringLiteral("unexpected argument '%1' after startup flags").arg(args.at(index));
        return inv;
    }
    if (index >= args.size())
        return inv;

    const QString first = args.at(index);

    if (first == QStringLiteral("help")) {
        inv.mode = CliInvocation::Mode::Help;
        return inv;
    }

    static const QStringList knownCommands = {
        QStringLiteral("status"), QStringLiteral("awake"), QStringLiteral("focus"),
        QStringLiteral("work"), QStringLiteral("export"), QStringLiteral("pause-all"),
        QStringLiteral("diagnostics"), QStringLiteral("open"), QStringLiteral("quit")
    };

    if (!knownCommands.contains(first)) {
        inv.mode = CliInvocation::Mode::Command;
        inv.error = QStringLiteral("unknown command '%1' (try: status, awake, diagnostics, open, quit)").arg(first);
        return inv;
    }

    inv.mode = CliInvocation::Mode::Command;
    inv.command = first;

    for (int i = index + 1; i < args.size(); ++i) {
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
