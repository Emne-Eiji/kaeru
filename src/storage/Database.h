// Kaeru — SQLite database handle (spec §10, ADR-004)
// WAL journaling, foreign keys ON, parameterized access only.
#pragma once
#include <QSqlDatabase>
#include <QString>

namespace kaeru {

class Database final {
public:
    // Opens Paths::databaseFile() (honors KAERU_TEST_HOME sandbox).
    Database();
    // Opens an explicit file (used by tests and backup tooling).
    explicit Database(const QString& filePath);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool open(QString* error = nullptr);
    bool isOpen() const { return m_db.isValid() && m_db.isOpen(); }
    QSqlDatabase& handle() { return m_db; }
    const QSqlDatabase& handle() const { return m_db; }
    QString filePath() const { return m_filePath; }
    QString lastError() const { return m_lastError; }

    // Quick integrity verification (PRAGMA quick_check).
    bool integrityOk(QString* detail = nullptr) const;

private:
    QString m_filePath;
    QSqlDatabase m_db;
    QString m_lastError;
    QString m_connectionName;
};

} // namespace kaeru
