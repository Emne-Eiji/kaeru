// Kaeru — duration parsing and formatting (spec §6.1, §12)
#pragma once
#include <QDate>
#include <QString>

namespace kaeru {

// Parses "90s", "30m", "2h", "1h30m", "25" (seconds). Returns false on
// malformed input (rejected, never guessed). Result in milliseconds.
bool parseDurationMs(const QString& text, qint64* resultMs);

// Parses "YYYY-MM-DD" local date. Returns false on malformed input.
bool parseIsoDate(const QString& text, QDate* result);

QString formatDurationMs(qint64 ms);        // "1h 05m 09s" style
QString formatRemainingMs(qint64 ms);       // short form "18m", "45s", "2h 15m"

} // namespace kaeru
