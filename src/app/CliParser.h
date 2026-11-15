// Kaeru — CLI argument parser (spec §12, ADR-005)
#pragma once
#include <QDate>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace kaeru {

struct CliInvocation {
    enum class Mode : quint8 { Gui, GuiBackground, Command, Help, Version };

    Mode mode = Mode::Gui;
    QString command;            // e.g. "status", "awake"
    QStringList operands;       // sub-operands e.g. ["on"]
    QVariantMap options;        // parsed --key value pairs
    QString error;              // usage error message when invalid

    bool isUsageError() const { return !error.isEmpty(); }
};

class CliParser final {
public:
    // Parses argv (without argv[0]). Rejects malformed values explicitly.
    static CliInvocation parse(const QStringList& args);

private:
    static bool parseDurationOption(const QString& value, qint64* ms);
    static bool parseDateOption(const QString& value, QDate* d);
    static bool parseModeOption(const QString& value, QString* mode);
};

} // namespace kaeru
