// Kaeru — clock sources (ADR-003)
// Monotonic: suspend-exclusive (work/focus accrual)
// Boot: suspend-inclusive (countdowns) — Linux CLOCK_BOOTTIME
// Wall: UTC wall clock (deadlines, timestamps)
#pragma once
#include <QDateTime>
#include <QObject>
#include <QTimeZone>

namespace kaeru {

enum class ClockKind : quint8 { Monotonic, Boot, Wall };

class TimeSource {
public:
    virtual ~TimeSource() = default;
    virtual qint64 nowMs() const = 0;
    virtual ClockKind kind() const = 0;
};

// CLOCK_MONOTONIC (via QElapsedTimer semantics): does not run during suspend.
class MonotonicTimeSource final : public TimeSource {
public:
    MonotonicTimeSource();
    qint64 nowMs() const override;
    ClockKind kind() const override { return ClockKind::Monotonic; }

private:
    class Impl;
    Impl* d;
};

// CLOCK_BOOTTIME: runs during suspend. Falls back to monotonic when
// unavailable (capability is then reported honestly, ADR-003).
class BootTimeSource final : public TimeSource {
public:
    BootTimeSource();
    qint64 nowMs() const override;
    ClockKind kind() const override { return ClockKind::Boot; }
    static bool available(); // true when CLOCK_BOOTTIME works on this kernel

private:
    class Impl;
    Impl* d;
};

// Wall clock; injectable timezone for tests.
class WallTimeSource final : public TimeSource {
public:
    explicit WallTimeSource(const QString& timeZoneId = QString());
    qint64 nowMs() const override;
    ClockKind kind() const override { return ClockKind::Wall; }
    QDateTime nowUtc() const;
    QDateTime nowLocal() const;
    const QTimeZone& timeZone() const { return m_tz; }

private:
    QTimeZone m_tz;
};

// Deterministic sources for tests
class FakeTimeSource final : public TimeSource {
public:
    explicit FakeTimeSource(qint64 startMs = 0) : m_now(startMs) {}
    qint64 nowMs() const override { return m_now; }
    ClockKind kind() const override { return ClockKind::Monotonic; }
    void advanceMs(qint64 ms) { m_now += ms; }
    void setNowMs(qint64 ms) { m_now = ms; }

private:
    qint64 m_now;
};

// Concrete clock set used by engines; tests may replace members.
struct ClockSet {
    TimeSource* monotonic = nullptr;
    TimeSource* boot = nullptr;
    TimeSource* wall = nullptr;
};

} // namespace kaeru
