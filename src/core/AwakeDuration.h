// Kaeru — Keep Awake duration selection (WP-08)
//
// A finite, validated duration or the distinct "Indefinitely" choice. The
// main GUI and the tray both use this model so validation, persistence and
// restart semantics cannot diverge.
//
// Custom durations are expressed in whole hours and minutes. The maximum is
// seven days, matching the existing CLI duration limit (Duration.cpp); it is
// a finite timer, never a substitute for "Indefinitely".
#pragma once

#include <QString>
#include <QtGlobal>

namespace kaeru {

constexpr qint64 kMinCustomAwakeMs = 60LL * 1000;
constexpr qint64 kMaxCustomAwakeMs = 7LL * 24 * 60 * 60 * 1000;

struct AwakeDuration {
    enum class Kind : quint8 {
        Minutes15,
        Minutes30,
        Hour1,
        Hours2,
        Indefinite,
        Custom,
    };

    Kind kind = Kind::Hour1;
    qint64 customMs = 0; // valid only for Kind::Custom

    static AwakeDuration standard(Kind kind);
    static bool custom(qint64 hours, qint64 minutes, AwakeDuration* out,
                       QString* error = nullptr);

    // Stable persistence token: "15m", "30m", "1h", "2h", "indefinite" or
    // "custom:<milliseconds>". Unknown/corrupt tokens are rejected so callers
    // can fall back to a safe default instead of guessing.
    static bool fromToken(const QString& token, AwakeDuration* out);
    QString token() const;

    bool isIndefinite() const { return kind == Kind::Indefinite; }
    bool isCustom() const { return kind == Kind::Custom; }

    // -1 for Indefinitely, otherwise a finite positive duration in ms.
    qint64 durationMs() const;

    // Human-readable English label ("1 hour", "Custom · 1h 30m").
    QString label() const;
};

// Validates whole hours/minutes and returns the finite duration in ms.
// Rejects zero, negative, malformed (handled by the spin boxes) and values
// above the documented maximum. The error text is user-facing.
bool validateCustomAwakeDuration(qint64 hours, qint64 minutes, qint64* outMs,
                                 QString* error = nullptr);

// "1h 30m" / "45m" — compact, unambiguous custom-duration display.
QString formatAwakeDuration(qint64 ms);

} // namespace kaeru
