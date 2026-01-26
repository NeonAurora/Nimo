#include "repositories/streakrepository.h"
#include "logging/logger.h"
#include "logging/requestscope.h"
#include "logging/loggermacros.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QUuid>

StreakRepository::StreakRepository(QSqlDatabase db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
}

Streak* StreakRepository::create(const Streak& streak)
{
    RequestScope scope("StreakRepository::create", "CREATE", {
                                                                 {"scope", streak.scope}
                                                             });

    QString sql = R"(
        INSERT INTO streaks (
            id, goal_id, scope, current_streak, longest_streak,
            last_success_date, last_break_date,
            total_successes, total_failures, success_rate
        ) VALUES (
            :id, :goal_id, :scope, :current, :longest,
            :success_date, :break_date,
            :successes, :failures, :rate
        ) RETURNING id
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);

    QString streakId = streak.id.isEmpty() ?
                           QUuid::createUuid().toString(QUuid::WithoutBraces) : streak.id;

    query.bindValue(":id", streakId);
    query.bindValue(":goal_id", streak.goalId.isEmpty() ? QVariant(QVariant::String) : streak.goalId);
    query.bindValue(":scope", streak.scope);
    query.bindValue(":current", streak.currentStreak);
    query.bindValue(":longest", streak.longestStreak);
    query.bindValue(":success_date", streak.lastSuccessDate.isValid() ?
                                         streak.lastSuccessDate : QVariant(QVariant::Date));
    query.bindValue(":break_date", streak.lastBreakDate.isValid() ?
                                       streak.lastBreakDate : QVariant(QVariant::Date));
    query.bindValue(":successes", streak.totalSuccesses);
    query.bindValue(":failures", streak.totalFailures);
    query.bindValue(":rate", streak.successRate);

    LOG_QUERY(scope.requestId(), sql, {streakId, streak.scope});

    if (!query.exec() || !query.next()) {
        scope.logError(query.lastError().text(), "DB_INSERT_FAILED");
        return nullptr;
    }

    QString newId = query.value(0).toString();
    scope.logSuccess({{"streakId", newId}});

    return findById(newId);
}

Streak* StreakRepository::findById(const QString& id)
{
    QString sql = "SELECT * FROM streaks WHERE id = :id";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return nullptr;
    }

    return mapFromRecord(query.record());
}

Streak* StreakRepository::findByGoalAndScope(const QString& goalId, const QString& scope)
{
    QString sql = "SELECT * FROM streaks WHERE goal_id = :goal_id AND scope = :scope";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":goal_id", goalId);
    query.bindValue(":scope", scope);

    if (!query.exec() || !query.next()) {
        return nullptr;
    }

    return mapFromRecord(query.record());
}

Streak* StreakRepository::findOverallByScope(const QString& scope)
{
    QString sql = "SELECT * FROM streaks WHERE goal_id IS NULL AND scope = :scope";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":scope", scope);

    if (!query.exec() || !query.next()) {
        return nullptr;
    }

    return mapFromRecord(query.record());
}

bool StreakRepository::update(const Streak& streak)
{
    RequestScope scope("StreakRepository::update", "UPDATE", {
                                                                 {"streakId", streak.id}
                                                             });

    QString sql = R"(
        UPDATE streaks SET
            current_streak = :current,
            longest_streak = :longest,
            last_success_date = :success_date,
            last_break_date = :break_date,
            total_successes = :successes,
            total_failures = :failures,
            success_rate = :rate,
            updated_at = CURRENT_TIMESTAMP
        WHERE id = :id
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":id", streak.id);
    query.bindValue(":current", streak.currentStreak);
    query.bindValue(":longest", streak.longestStreak);
    query.bindValue(":success_date", streak.lastSuccessDate.isValid() ?
                                         streak.lastSuccessDate : QVariant(QVariant::Date));
    query.bindValue(":break_date", streak.lastBreakDate.isValid() ?
                                       streak.lastBreakDate : QVariant(QVariant::Date));
    query.bindValue(":successes", streak.totalSuccesses);
    query.bindValue(":failures", streak.totalFailures);
    query.bindValue(":rate", streak.successRate);

    LOG_QUERY(scope.requestId(), sql, {streak.id});

    if (!query.exec()) {
        scope.logError(query.lastError().text(), "DB_UPDATE_FAILED");
        return false;
    }

    scope.logSuccess({{"streakId", streak.id}});
    return true;
}

Streak* StreakRepository::getOrCreate(const QString& goalId, const QString& scope)
{
    Streak* existing = findByGoalAndScope(goalId, scope);
    if (existing) {
        return existing;
    }

    Streak streak;
    streak.goalId = goalId;
    streak.scope = scope;
    streak.currentStreak = 0;
    streak.longestStreak = 0;
    streak.totalSuccesses = 0;
    streak.totalFailures = 0;
    streak.successRate = 0.0;

    return create(streak);
}

Streak* StreakRepository::getOrCreateOverall(const QString& scope)
{
    Streak* existing = findOverallByScope(scope);
    if (existing) {
        return existing;
    }

    Streak streak;
    streak.goalId = "";  // NULL for overall
    streak.scope = scope;
    streak.currentStreak = 0;
    streak.longestStreak = 0;
    streak.totalSuccesses = 0;
    streak.totalFailures = 0;
    streak.successRate = 0.0;

    return create(streak);
}

Streak* StreakRepository::mapFromRecord(const QSqlRecord& record)
{
    Streak* streak = new Streak();
    streak->id = record.value("id").toString();
    streak->goalId = record.value("goal_id").toString();
    streak->scope = record.value("scope").toString();
    streak->currentStreak = record.value("current_streak").toInt();
    streak->longestStreak = record.value("longest_streak").toInt();
    streak->lastSuccessDate = record.value("last_success_date").toDate();
    streak->lastBreakDate = record.value("last_break_date").toDate();
    streak->totalSuccesses = record.value("total_successes").toInt();
    streak->totalFailures = record.value("total_failures").toInt();
    streak->successRate = record.value("success_rate").toDouble();

    return streak;
}
