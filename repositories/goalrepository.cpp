#include "repositories/goalrepository.h"
#include "logging/logger.h"
#include "logging/requestscope.h"
#include "logging/loggermacros.h"
#include <QSqlRecord>
#include <QSqlError>
#include <QVariant>
#include <QUuid>

GoalRepository::GoalRepository(QSqlDatabase db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
}

Goal* GoalRepository::create(const Goal& goal)
{
    RequestScope scope("GoalRepository::create", "CREATE", {
                                                               {"title", goal.title},
                                                               {"scope", goal.scope},
                                                               {"points", goal.points}
                                                           });

    QString sql = R"(
        INSERT INTO goals (
            id, title, scope, points, missing_behavior, penalty_points,
            category, notes, icon_name, color_hex, sort_order, is_active
        ) VALUES (
            :id, :title, :scope, :points, :behavior, :penalty,
            :category, :notes, :icon, :color, :order, :active
        ) RETURNING id
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);

    // Generate UUID if not provided
    QString goalId = goal.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : goal.id;
    query.bindValue(":id", goalId);
    bindGoalValues(query, goal);

    LOG_QUERY(scope.requestId(), sql, {
                                          goalId, goal.title, goal.scope, goal.points
                                      });

    if (!query.exec()) {
        scope.logError(query.lastError().text(), "DB_INSERT_FAILED");
        return nullptr;
    }

    if (!query.next()) {
        scope.logError("No ID returned after insert", "DB_INSERT_FAILED");
        return nullptr;
    }

    QString newId = query.value(0).toString();

    scope.logSuccess({
        {"goalId", newId},
        {"rowsAffected", 1}
    });

    emit goalCreated(newId);

    // Fetch and return the created goal
    return findById(newId);
}

Goal* GoalRepository::findById(const QString& id)
{
    RequestScope scope("GoalRepository::findById", "READ", {
                                                               {"goalId", id}
                                                           });

    QString sql = "SELECT * FROM goals WHERE id = :id AND deleted_at IS NULL";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":id", id);

    LOG_QUERY(scope.requestId(), sql, {id});

    if (!query.exec()) {
        scope.logError(query.lastError().text(), "SQL_EXEC_FAILED");
        return nullptr;
    }

    if (!query.next()) {
        scope.logError("Goal not found", "NOT_FOUND");
        return nullptr;
    }

    Goal* goal = mapFromRecord(query.record());

    scope.logSuccess({
        {"goalId", goal->id},
        {"title", goal->title}
    });

    return goal;
}

QList<Goal*> GoalRepository::findAll()
{
    RequestScope scope("GoalRepository::findAll", "READ", {});

    QString sql = "SELECT * FROM goals WHERE deleted_at IS NULL ORDER BY scope, sort_order, created_at";

    QSqlQuery query(m_db);
    LOG_QUERY(scope.requestId(), sql, {});

    if (!query.exec()) {
        scope.logError(query.lastError().text(), "SQL_EXEC_FAILED");
        return QList<Goal*>();
    }

    QList<Goal*> goals;
    while (query.next()) {
        goals.append(mapFromRecord(query.record()));
    }

    scope.logSuccess({
        {"count", goals.size()}
    });

    return goals;
}

QList<Goal*> GoalRepository::findByScope(const QString& scope)
{
    RequestScope reqScope("GoalRepository::findByScope", "READ", {
                                                                     {"scope", scope}
                                                                 });

    QString sql = "SELECT * FROM goals WHERE scope = :scope AND deleted_at IS NULL "
                  "ORDER BY sort_order, created_at";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":scope", scope);

    LOG_QUERY(reqScope.requestId(), sql, {scope});

    if (!query.exec()) {
        reqScope.logError(query.lastError().text(), "SQL_EXEC_FAILED");
        return QList<Goal*>();
    }

    QList<Goal*> goals;
    while (query.next()) {
        goals.append(mapFromRecord(query.record()));
    }

    reqScope.logSuccess({
        {"scope", scope},
        {"count", goals.size()}
    });

    return goals;
}

QList<Goal*> GoalRepository::findActiveGoals()
{
    RequestScope scope("GoalRepository::findActiveGoals", "READ", {});

    QString sql = "SELECT * FROM goals WHERE is_active = true AND deleted_at IS NULL "
                  "ORDER BY scope, sort_order, created_at";

    QSqlQuery query(m_db);
    LOG_QUERY(scope.requestId(), sql, {});

    if (!query.exec()) {
        scope.logError(query.lastError().text(), "SQL_EXEC_FAILED");
        return QList<Goal*>();
    }

    QList<Goal*> goals;
    while (query.next()) {
        goals.append(mapFromRecord(query.record()));
    }

    scope.logSuccess({
        {"count", goals.size()}
    });

    return goals;
}

bool GoalRepository::update(const Goal& goal)
{
    RequestScope scope("GoalRepository::update", "UPDATE", {
                                                               {"goalId", goal.id},
                                                               {"title", goal.title}
                                                           });

    QString sql = R"(
        UPDATE goals SET
            title = :title,
            scope = :scope,
            points = :points,
            missing_behavior = :behavior,
            penalty_points = :penalty,
            category = :category,
            notes = :notes,
            icon_name = :icon,
            color_hex = :color,
            sort_order = :order,
            is_active = :active,
            updated_at = CURRENT_TIMESTAMP
        WHERE id = :id AND deleted_at IS NULL
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":id", goal.id);
    bindGoalValues(query, goal);

    LOG_QUERY(scope.requestId(), sql, {goal.id, goal.title});

    if (!query.exec()) {
        scope.logError(query.lastError().text(), "DB_UPDATE_FAILED");
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    if (rowsAffected == 0) {
        scope.logError("Goal not found or already deleted", "NOT_FOUND");
        return false;
    }

    scope.logSuccess({
        {"goalId", goal.id},
        {"rowsAffected", rowsAffected}
    });

    emit goalUpdated(goal.id);
    return true;
}

bool GoalRepository::softDelete(const QString& id)
{
    RequestScope scope("GoalRepository::softDelete", "DELETE", {
                                                                   {"goalId", id}
                                                               });

    QString sql = "UPDATE goals SET deleted_at = CURRENT_TIMESTAMP WHERE id = :id AND deleted_at IS NULL";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":id", id);

    LOG_QUERY(scope.requestId(), sql, {id});

    if (!query.exec()) {
        scope.logError(query.lastError().text(), "DB_DELETE_FAILED");
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    if (rowsAffected == 0) {
        scope.logError("Goal not found or already deleted", "NOT_FOUND");
        return false;
    }

    scope.logSuccess({
        {"goalId", id},
        {"rowsAffected", rowsAffected}
    });

    emit goalDeleted(id);
    return true;
}

bool GoalRepository::hardDelete(const QString& id)
{
    RequestScope scope("GoalRepository::hardDelete", "DELETE", {
                                                                   {"goalId", id}
                                                               });

    QString sql = "DELETE FROM goals WHERE id = :id";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":id", id);

    LOG_QUERY(scope.requestId(), sql, {id});

    if (!query.exec()) {
        scope.logError(query.lastError().text(), "DB_DELETE_FAILED");
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    if (rowsAffected == 0) {
        scope.logError("Goal not found", "NOT_FOUND");
        return false;
    }

    scope.logSuccess({
        {"goalId", id},
        {"rowsAffected", rowsAffected}
    });

    emit goalDeleted(id);
    return true;
}

int GoalRepository::countByScope(const QString& scope)
{
    QString sql = "SELECT COUNT(*) FROM goals WHERE scope = :scope AND deleted_at IS NULL";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":scope", scope);

    if (!query.exec() || !query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

bool GoalRepository::exists(const QString& id)
{
    QString sql = "SELECT EXISTS(SELECT 1 FROM goals WHERE id = :id AND deleted_at IS NULL)";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return false;
    }

    return query.value(0).toBool();
}

Goal* GoalRepository::mapFromRecord(const QSqlRecord& record)
{
    Goal* goal = new Goal();
    goal->id = record.value("id").toString();
    goal->title = record.value("title").toString();
    goal->scope = record.value("scope").toString();
    goal->points = record.value("points").toInt();
    goal->missingBehavior = record.value("missing_behavior").toString();
    goal->penaltyPoints = record.value("penalty_points").toInt();
    goal->category = record.value("category").toString();
    goal->notes = record.value("notes").toString();
    goal->iconName = record.value("icon_name").toString();
    goal->colorHex = record.value("color_hex").toString();
    goal->sortOrder = record.value("sort_order").toInt();
    goal->isActive = record.value("is_active").toBool();

    return goal;
}

void GoalRepository::bindGoalValues(QSqlQuery& query, const Goal& goal)
{
    query.bindValue(":title", goal.title);
    query.bindValue(":scope", goal.scope);
    query.bindValue(":points", goal.points);
    query.bindValue(":behavior", goal.missingBehavior);
    query.bindValue(":penalty", goal.penaltyPoints);
    query.bindValue(":category", goal.category);
    query.bindValue(":notes", goal.notes);
    query.bindValue(":icon", goal.iconName);
    query.bindValue(":color", goal.colorHex);
    query.bindValue(":order", goal.sortOrder);
    query.bindValue(":active", goal.isActive);
}
