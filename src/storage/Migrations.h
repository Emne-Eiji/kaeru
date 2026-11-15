// Kaeru — versioned schema migrations (spec §10, ADR-004)
// Every migration runs inside a transaction and is recorded in
// schema_migrations; risky migrations must back up the file first.
#pragma once
#include <QSqlDatabase>
#include <QString>

namespace kaeru {

class Migrations final {
public:
    static constexpr int kLatestVersion = 1;

    // Applies all pending migrations. Returns false with details on error.
    static bool apply(QSqlDatabase& db, QString* error = nullptr);
    static int currentVersion(QSqlDatabase& db);
    // Backs up the database file before a risky migration (spec §10).
    static bool backupFile(const QString& databasePath, QString* backupPath, QString* error);

private:
    static bool migrateV1(QSqlDatabase& db, QString* error);
};

} // namespace kaeru
