// Kaeru — project repository (spec §10)
// All statements parameterized; names are case-insensitive unique.
#pragma once
#include <QDateTime>
#include <QList>
#include <QSqlDatabase>
#include <QString>

namespace kaeru {

struct ProjectRecord {
    qint64 id = 0;
    QString name;
    QString color;
    QDateTime createdAtUtc;
    bool archived = false;
};

class ProjectRepository final {
public:
    explicit ProjectRepository(QSqlDatabase db);

    // Ensures the default project exists ("No project"); returns its id.
    bool ensureDefaultProject(const QString& name, qint64* idOut, QString* error = nullptr);

    QList<ProjectRecord> list(bool includeArchived = true, QString* error = nullptr) const;
    bool byId(qint64 id, ProjectRecord* out, QString* error = nullptr) const;
    bool byName(const QString& name, ProjectRecord* out, QString* error = nullptr) const;

    qint64 add(const QString& name, const QString& color, QString* error = nullptr);
    bool rename(qint64 id, const QString& newName, QString* error = nullptr);
    bool setArchived(qint64 id, bool archived, QString* error = nullptr);
    // Removes only projects with no recorded sessions (never invents/deletes work).
    bool remove(qint64 id, QString* error = nullptr);

private:
    QSqlDatabase m_db;
};

} // namespace kaeru
