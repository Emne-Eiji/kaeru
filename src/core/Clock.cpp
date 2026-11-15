#include "Clock.h"

#include <QElapsedTimer>

#ifdef Q_OS_LINUX
#include <time.h>
#endif

namespace kaeru {

class MonotonicTimeSource::Impl {
public:
    QElapsedTimer timer;
};

MonotonicTimeSource::MonotonicTimeSource() : d(new Impl)
{
    d->timer.start();
}

qint64 MonotonicTimeSource::nowMs() const
{
    return d->timer.elapsed();
}

class BootTimeSource::Impl {
public:
    bool ok = false;
    QElapsedTimer fallback; // used only if CLOCK_BOOTTIME unavailable
#ifdef Q_OS_LINUX
    struct timespec origin {};
#endif
};

BootTimeSource::BootTimeSource() : d(new Impl)
{
    d->fallback.start();
#ifdef Q_OS_LINUX
    struct timespec ts;
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
        d->origin = ts;
        d->ok = true;
    }
#endif
}

qint64 BootTimeSource::nowMs() const
{
#ifdef Q_OS_LINUX
    if (d->ok) {
        struct timespec ts;
        if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
            qint64 sec = static_cast<qint64>(ts.tv_sec) - static_cast<qint64>(d->origin.tv_sec);
            qint64 nsec = static_cast<qint64>(ts.tv_nsec) - static_cast<qint64>(d->origin.tv_nsec);
            if (nsec < 0) { nsec += 1000000000LL; sec -= 1; }
            return sec * 1000 + nsec / 1000000;
        }
    }
#endif
    // Honest fallback: behave as monotonic (countdown will pause across
    // suspend). Callers must check BootTimeSource::available() and report
    // the limitation (ADR-003); we never silently pretend.
    return d->fallback.elapsed();
}

bool BootTimeSource::available()
{
#ifdef Q_OS_LINUX
    struct timespec ts;
    return clock_gettime(CLOCK_BOOTTIME, &ts) == 0;
#else
    return false;
#endif
}

WallTimeSource::WallTimeSource(const QString& timeZoneId)
{
    if (timeZoneId.isEmpty())
        m_tz = QTimeZone::systemTimeZone();
    else
        m_tz = QTimeZone(timeZoneId.toUtf8());
    if (!m_tz.isValid())
        m_tz = QTimeZone::utc();
}

qint64 WallTimeSource::nowMs() const
{
    return QDateTime::currentMSecsSinceEpoch();
}

QDateTime WallTimeSource::nowUtc() const
{
    return QDateTime::currentDateTimeUtc();
}

QDateTime WallTimeSource::nowLocal() const
{
    return QDateTime::currentDateTime().toTimeZone(m_tz);
}

} // namespace kaeru
