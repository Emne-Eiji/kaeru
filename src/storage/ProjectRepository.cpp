// Kaeru — project repository (spec §10)
#include "ProjectRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace kaeru {

ProjectRepository::ProjectRepository(QSqlDatabase db)
    : m_db(db)
{
}

bool ProjectRepository::ensureDefaultProject(const QString& name, qint64* idOut, QString* error)
{
    ProjectRecord p;
    if (byName(name, &p, error)) {
        if (idOut)
            *idOut = p.id;
        return true;
    }
    const qint64 id = add(name, QString(), error);
    if (idOut)
        *idOut = id;
    return id > 0;
}

QList<ProjectRecord> ProjectRepository::list(bool includeArchived, QString* error) const
{
    QList<ProjectRecord> out;
    QSqlQuery q(m_db);
    const QString sql = includeArchived
        ? QStringLiteral("SELECT id, name, color, archived, created_at FROM projects ORDER BY name COLLATE NOCASE")
        : QStringLiteral("SELECT id, name, color, archived, created_at FROM projects WHERE archived = 0 ORDER BY name COLLATE NOCASE");
    if (!q.exec(sql)) {
        if (error)
            *error = q.lastError().text();
        return out;
    }
    while (q.next()) {
        ProjectRecord p;
        p.id = q.value(0).toLongLong();
        p.name = q.value(1).toString();
        p.color = q.value(2).toString();
        p.archived = q.value(3).toInt() != 0;
        p.createdAtUtc = QDateTime::fromString(q.value(4).toString(), Qt::ISODate).toUTC();
        out.append(p);
    }
    return out;
}

bool ProjectRepository::byId(qint64 id, ProjectRecord* out, QString* error) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, name, color, archived, created_at FROM projects WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    if (!q.next())
        return false;
    if (out) {
        out->id = q.value(0).toLongLong();
        out->name = q.value(1).toString();
        out->color = q.value(2).toString();
        out->archived = q.value(3).toInt() != 0;
        out->createdAtUtc = QDateTime::fromString(q.value(4).toString(), Qt::ISODate).toUTC();
    }
    return true;
}

bool ProjectRepository::byName(const QString& name, ProjectRecord* out, QString* error) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, name, color, archived, created_at FROM projects WHERE name = :name COLLATE NOCASE"));
    q.bindValue(QStringLiteral(":name"), name);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    if (!q.next())
        return false;
    if (out) {
        out->id = q.value(0).toLongLong();
        out->name = q.value(1).toString();
        out->color = q.value(2).toString();
        out->archived = q.value(3).toInt() != 0;
        out->createdAtUtc = QDateTime::fromString(q.value(4).toString(), Qt::ISODate).toUTC();
    }
    return true;
}

qint64 ProjectRepository::add(const QString& name, const QString& color, QString* error)
{
    if (name.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("project name must not be empty");
        return 0;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO projects (name, color, created_at) VALUES (:name, :color, :created)"));
    q.bindValue(QStringLiteral(":name"), name.trimmed());
    q.bindValue(QStringLiteral(":color"), color);
    q.bindValue(QStringLiteral(":created"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return 0;
    }
    return q.lastInsertId().toLongLong();
}

bool ProjectRepository::rename(qint64 id, const QString& newName, QString* error)
{
    if (newName.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("project name must not be empty");
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE projects SET name = :name WHERE id = :id"));
    q.bindValue(QStringLiteral(":name"), newName.trimmed());
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool ProjectRepository::setArchived(qint64 id, bool archived, QString* error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE projects SET archived = :arch WHERE id = :id"));
    q.bindValue(QStringLiteral(":arch"), archived ? 1 : 0);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool ProjectRepository::remove(qint64 id, QString* error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM sessions WHERE project_id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    if (q.value(0).toLongLong() > 0) {
        if (error)
            *error = QStringLiteral("project has recorded sessions; archive instead");
        return false;
    }
    QSqlQuery del(m_db);
    del.prepare(QStringLiteral("DELETE FROM projects WHERE id = :id"));
    del.bindValue(QStringLiteral(":id"), id);
    if (!del.exec()) {
        if (error)
            *error = del.lastError().text();
        return false;
    }
    return del.numRowsAffected() == 1;
}

} // namespace kaeru
