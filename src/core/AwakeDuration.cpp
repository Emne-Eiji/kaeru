#include "AwakeDuration.h"

#include <QCoreApplication>

namespace kaeru {

namespace {

QString customError(qint64 hours, qint64 minutes)
{
    if (hours < 0 || minutes < 0 || minutes > 59)
        return QCoreApplication::translate("AwakeDuration",
            "Enter a whole number of hours (0–168) and minutes (0–59).");
    if (hours > 168)
        return QCoreApplication::translate("AwakeDuration",
            "Custom duration cannot exceed 7 days (168 hours).");
    if (hours == 0 && minutes == 0)
        return QCoreApplication::translate("AwakeDuration",
            "Custom duration must be at least 1 minute.");
    if (hours == 168 && minutes > 0)
        return QCoreApplication::translate("AwakeDuration",
            "Custom duration cannot exceed 7 days (168 hours).");
    return QString();
}

} // namespace

AwakeDuration AwakeDuration::standard(Kind kind)
{
    AwakeDuration d;
    d.kind = kind;
    return d;
}

bool AwakeDuration::custom(qint64 hours, qint64 minutes, AwakeDuration* out,
                           QString* error)
{
    qint64 ms = 0;
    if (!validateCustomAwakeDuration(hours, minutes, &ms, error))
        return false;
    if (out) {
        out->kind = Kind::Custom;
        out->customMs = ms;
    }
    return true;
}

bool AwakeDuration::fromToken(const QString& token, AwakeDuration* out)
{
    if (!out)
        return false;
    const QString t = token.trimmed();
    AwakeDuration d;
    if (t == QStringLiteral("15m")) {
        d.kind = Kind::Minutes15;
    } else if (t == QStringLiteral("30m")) {
        d.kind = Kind::Minutes30;
    } else if (t == QStringLiteral("1h")) {
        d.kind = Kind::Hour1;
    } else if (t == QStringLiteral("2h")) {
        d.kind = Kind::Hours2;
    } else if (t == QStringLiteral("indefinite")) {
        d.kind = Kind::Indefinite;
    } else if (t.startsWith(QStringLiteral("custom:"))) {
        bool ok = false;
        const qint64 ms = t.mid(7).toLongLong(&ok);
        if (!ok || ms < kMinCustomAwakeMs || ms > kMaxCustomAwakeMs
            || ms % (60LL * 1000) != 0)
            return false;
        d.kind = Kind::Custom;
        d.customMs = ms;
    } else {
        return false;
    }
    *out = d;
    return true;
}

QString AwakeDuration::token() const
{
    switch (kind) {
    case Kind::Minutes15: return QStringLiteral("15m");
    case Kind::Minutes30: return QStringLiteral("30m");
    case Kind::Hour1: return QStringLiteral("1h");
    case Kind::Hours2: return QStringLiteral("2h");
    case Kind::Indefinite: return QStringLiteral("indefinite");
    case Kind::Custom:
        return QStringLiteral("custom:%1").arg(customMs);
    }
    return QStringLiteral("1h");
}

qint64 AwakeDuration::durationMs() const
{
    switch (kind) {
    case Kind::Minutes15: return 15LL * 60 * 1000;
    case Kind::Minutes30: return 30LL * 60 * 1000;
    case Kind::Hour1: return 60LL * 60 * 1000;
    case Kind::Hours2: return 120LL * 60 * 1000;
    case Kind::Indefinite: return -1;
    case Kind::Custom:
        return (customMs >= kMinCustomAwakeMs && customMs <= kMaxCustomAwakeMs)
            ? customMs : -1;
    }
    return -1;
}

QString AwakeDuration::label() const
{
    switch (kind) {
    case Kind::Minutes15:
        return QCoreApplication::translate("AwakeDuration", "15 minutes");
    case Kind::Minutes30:
        return QCoreApplication::translate("AwakeDuration", "30 minutes");
    case Kind::Hour1:
        return QCoreApplication::translate("AwakeDuration", "1 hour");
    case Kind::Hours2:
        return QCoreApplication::translate("AwakeDuration", "2 hours");
    case Kind::Indefinite:
        return QCoreApplication::translate("AwakeDuration", "Indefinitely");
    case Kind::Custom:
        return QCoreApplication::translate("AwakeDuration", "Custom · %1")
            .arg(formatAwakeDuration(customMs));
    }
    return QCoreApplication::translate("AwakeDuration", "1 hour");
}

bool validateCustomAwakeDuration(qint64 hours, qint64 minutes, qint64* outMs,
                                 QString* error)
{
    const QString err = customError(hours, minutes);
    if (!err.isEmpty()) {
        if (error)
            *error = err;
        return false;
    }
    const qint64 ms = ((hours * 60 + minutes) * 60) * 1000;
    if (ms < kMinCustomAwakeMs || ms > kMaxCustomAwakeMs) {
        if (error)
            *error = customError(168, 1);
        return false;
    }
    if (outMs)
        *outMs = ms;
    if (error)
        error->clear();
    return true;
}

QString formatAwakeDuration(qint64 ms)
{
    if (ms <= 0)
        return QStringLiteral("0m");
    const qint64 totalMinutes = (ms + 59 * 1000) / (60 * 1000);
    const qint64 hours = totalMinutes / 60;
    const qint64 minutes = totalMinutes % 60;
    if (hours > 0 && minutes > 0)
        return QStringLiteral("%1h %2m").arg(hours).arg(minutes);
    if (hours > 0)
        return QStringLiteral("%1h").arg(hours);
    return QStringLiteral("%1m").arg(minutes);
}

} // namespace kaeru
