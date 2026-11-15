#include "Duration.h"

#include <QDate>

namespace kaeru {

bool parseDurationMs(const QString& text, qint64* resultMs)
{
    if (!resultMs)
        return false;
    const QString s = text.trimmed();
    if (s.isEmpty())
        return false;

    // Plain integer = seconds
    bool ok = false;
    const qint64 plain = s.toLongLong(&ok);
    if (ok) {
        if (plain <= 0 || plain > 60 * 60 * 24 * 7)
            return false;
        *resultMs = plain * 1000;
        return true;
    }

    // Composed form like 1h30m / 45m / 90s
    qint64 total = 0;
    qint64 currentValue = 0;
    bool sawUnit = false;
    bool digits = false;
    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s.at(i);
        if (c.isDigit()) {
            currentValue = currentValue * 10 + (c.unicode() - u'0');
            digits = true;
            if (currentValue > 100000)
                return false;
        } else if (c == u'h' || c == u'm' || c == u's') {
            if (!digits)
                return false;
            const qint64 mult = (c == u'h') ? 3600 : (c == u'm') ? 60 : 1;
            total += currentValue * mult;
            currentValue = 0;
            digits = false;
            sawUnit = true;
        } else if (c.isSpace()) {
            continue;
        } else {
            return false;
        }
    }
    if (currentValue != 0) // trailing digits without unit
        return false;
    if (!sawUnit || total <= 0 || total > 60 * 60 * 24 * 7)
        return false;
    *resultMs = total * 1000;
    return true;
}

bool parseIsoDate(const QString& text, QDate* result)
{
    if (!result)
        return false;
    const QDate d = QDate::fromString(text.trimmed(), QStringLiteral("yyyy-MM-dd"));
    if (!d.isValid())
        return false;
    *result = d;
    return true;
}

QString formatDurationMs(qint64 ms)
{
    if (ms < 0)
        ms = 0;
    const qint64 totalSec = (ms + 999) / 1000;
    const int h = int(totalSec / 3600);
    const int m = int((totalSec % 3600) / 60);
    const int s = int(totalSec % 60);
    if (h > 0)
        return QString::fromUtf8("%1h %2m %3s").arg(h).arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
    return QString::fromUtf8("%1m %2s").arg(m).arg(s, 2, 10, QLatin1Char('0'));
}

QString formatRemainingMs(qint64 ms)
{
    if (ms < 0)
        return QString::fromUtf8("\u2014");
    const qint64 totalSec = (ms + 999) / 1000;
    const int h = int(totalSec / 3600);
    const int m = int((totalSec % 3600) / 60);
    const int s = int(totalSec % 60);
    if (h > 0)
        return QString::fromUtf8("%1h %2m").arg(h).arg(m, 2, 10, QLatin1Char('0'));
    if (m > 0)
        return QString::fromUtf8("%1m %2s").arg(m).arg(s, 2, 10, QLatin1Char('0'));
    return QString::fromUtf8("%1s").arg(s);
}

} // namespace kaeru
