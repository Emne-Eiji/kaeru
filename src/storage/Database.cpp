// Kaeru — SQLite database handle (spec §10, ADR-004)
#include "Database.h"
#include "app/Paths.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

namespace kaeru {

Database::Database()
    : Database(Paths::databaseFile())
{
}

Database::Database(const QString& filePath)
    : m_filePath(filePath)
    , m_connectionName(QStringLiteral("kaeru-db-%1").arg(
          quintptr(this), 12, 16, QLatin1Char('0')))
{
}

Database::~Database()
{
    if (m_db.isValid()) {
        if (m_db.isOpen())
            m_db.close();
        m_db = QSqlDatabase(); // drop reference before removing connection
    }
    if (!m_connectionName.isEmpty())
        QSqlDatabase::removeDatabase(m_connectionName);
}

bool Database::open(QString* error)
{
    if (isOpen())
        return true;

    if (m_connectionName.isEmpty()) {
        m_connectionName = QStringLiteral("kaeru-db-%1").arg(
            quintptr(this), 12, 16, QLatin1Char('0'));
    }

    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(m_filePath);
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        if (error)
            *error = m_lastError;
        return false;
    }

    QSqlQuery q(m_db);
    // WAL: crash-safe writes with short recovery checkpoints (ADR-004).
    if (!q.exec(QStringLiteral("PRAGMA journal_mode=WAL"))) {
        m_lastError = q.lastError().text();
        if (error)
            *error = m_lastError;
        return false;
    }
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    q.exec(QStringLiteral("PRAGMA busy_timeout=5000"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    return true;
}

bool Database::integrityOk(QString* detail) const
{
    if (!isOpen()) {
        if (detail)
            *detail = QStringLiteral("database not open");
        return false;
    }
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("PRAGMA quick_check")) || !q.next()) {
        if (detail)
            *detail = q.lastError().text();
        return false;
    }
    const QString result = q.value(0).toString();
    if (detail)
        *detail = result;
    return result.compare(QStringLiteral("ok"), Qt::CaseInsensitive) == 0;
}

} // namespace kaeru
