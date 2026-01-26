#include "repositories/scorerepository.h"
#include "logging/logger.h"
#include "logging/requestscope.h"
#include "logging/loggermacros.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

ScoreRepository::ScoreRepository(QSqlDatabase db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
}

bool ScoreRepository::upsertDailyScore(const DailyScore& score)
{
    RequestScope scope("ScoreRepository::upsertDailyScore", "UPSERT", {
        {"date", score.date.toString("yyyy-MM-dd")}
    });

    QString sql = R"(
        INSERT INTO daily_scores (
            date, earned_score, target_score, completion_percentage,
            completed_count, skipped_count, not_completed_count, pending_count, total_count,
            perfect_day, has_negative_outcome
        ) VALUES (
            :date, :earned, :target, :percentage,
            :completed, :skipped, :not_completed, :pending, :total,
            :perfect, :negative
        )
        ON CONFLICT (date) DO UPDATE SET
            earned_score = EXCLUDED.earned_score,
            target_score = EXCLUDED.target_score,
            completion_percentage = EXCLUDED.completion_percentage,
            completed_count = EXCLUDED.completed_count,
            skipped_count = EXCLUDED.skipped_count,
            not_completed_count = EXCLUDED.not_completed_count,
            pending_count = EXCLUDED.pending_count,
            total_count = EXCLUDED.total_count,
            perfect_day = EXCLUDED.perfect_day,
            has_negative_outcome = EXCLUDED.has_negative_outcome,
            updated_at = CURRENT_TIMESTAMP
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":date", score.date);
    query.bindValue(":earned", score.earnedScore);
    query.bindValue(":target", score.targetScore);
    query.bindValue(":percentage", score.completionPercentage);
    query.bindValue(":completed", score.completedCount);
    query.bindValue(":skipped", score.skippedCount);
    query.bindValue(":not_completed", score.notCompletedCount);
    query.bindValue(":pending", score.pendingCount);
    query.bindValue(":total", score.totalCount);
    query.bindValue(":perfect", score.perfectDay);
    query.bindValue(":negative", score.hasNegativeOutcome);

    LOG_QUERY(scope.requestId(), sql, {score.date, score.earnedScore});

    if (!query.exec()) {
        scope.logError(query.lastError().text(), "DB_UPSERT_FAILED");
        return false;
    }

    scope.logSuccess({
        {"date", score.date.toString("yyyy-MM-dd")},
        {"earnedScore", score.earnedScore}
    });

    return true;
}

bool ScoreRepository::upsertWeeklyScore(const WeeklyScore& score)
{
    QString sql = R"(
        INSERT INTO weekly_scores (
            week_start, year, week_number, earned_score, target_score, completion_percentage,
            completed_count, skipped_count, not_completed_count, pending_count, total_count
        ) VALUES (
            :week_start, :year, :week_number, :earned, :target, :percentage,
            :completed, :skipped, :not_completed, :pending, :total
        )
        ON CONFLICT (week_start) DO UPDATE SET
            earned_score = EXCLUDED.earned_score,
            target_score = EXCLUDED.target_score,
            completion_percentage = EXCLUDED.completion_percentage,
            completed_count = EXCLUDED.completed_count,
            skipped_count = EXCLUDED.skipped_count,
            not_completed_count = EXCLUDED.not_completed_count,
            pending_count = EXCLUDED.pending_count,
            total_count = EXCLUDED.total_count,
            updated_at = CURRENT_TIMESTAMP
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":week_start", score.weekStart);
    query.bindValue(":year", score.year);
    query.bindValue(":week_number", score.weekNumber);
    query.bindValue(":earned", score.earnedScore);
    query.bindValue(":target", score.targetScore);
    query.bindValue(":percentage", score.completionPercentage);
    query.bindValue(":completed", score.completedCount);
    query.bindValue(":skipped", score.skippedCount);
    query.bindValue(":not_completed", score.notCompletedCount);
    query.bindValue(":pending", score.pendingCount);
    query.bindValue(":total", score.totalCount);

    return query.exec();
}

bool ScoreRepository::upsertMonthlyScore(const MonthlyScore& score)
{
    QString sql = R"(
        INSERT INTO monthly_scores (
            month_start, year, month, earned_score, target_score, completion_percentage,
            completed_count, skipped_count, not_completed_count, pending_count, total_count
        ) VALUES (
            :month_start, :year, :month, :earned, :target, :percentage,
            :completed, :skipped, :not_completed, :pending, :total
        )
        ON CONFLICT (month_start) DO UPDATE SET
            earned_score = EXCLUDED.earned_score,
            target_score = EXCLUDED.target_score,
            completion_percentage = EXCLUDED.completion_percentage,
            completed_count = EXCLUDED.completed_count,
            skipped_count = EXCLUDED.skipped_count,
            not_completed_count = EXCLUDED.not_completed_count,
            pending_count = EXCLUDED.pending_count,
            total_count = EXCLUDED.total_count,
            updated_at = CURRENT_TIMESTAMP
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":month_start", score.monthStart);
    query.bindValue(":year", score.year);
    query.bindValue(":month", score.month);
    query.bindValue(":earned", score.earnedScore);
    query.bindValue(":target", score.targetScore);
    query.bindValue(":percentage", score.completionPercentage);
    query.bindValue(":completed", score.completedCount);
    query.bindValue(":skipped", score.skippedCount);
    query.bindValue(":not_completed", score.notCompletedCount);
    query.bindValue(":pending", score.pendingCount);
    query.bindValue(":total", score.totalCount);

    return query.exec();
}

bool ScoreRepository::upsertYearlyScore(const YearlyScore& score)
{
    QString sql = R"(
        INSERT INTO yearly_scores (
            year_start, year, earned_score, target_score, completion_percentage,
            completed_count, skipped_count, not_completed_count, pending_count, total_count
        ) VALUES (
            :year_start, :year, :earned, :target, :percentage,
            :completed, :skipped, :not_completed, :pending, :total
        )
        ON CONFLICT (year_start) DO UPDATE SET
            earned_score = EXCLUDED.earned_score,
            target_score = EXCLUDED.target_score,
            completion_percentage = EXCLUDED.completion_percentage,
            completed_count = EXCLUDED.completed_count,
            skipped_count = EXCLUDED.skipped_count,
            not_completed_count = EXCLUDED.not_completed_count,
            pending_count = EXCLUDED.pending_count,
            total_count = EXCLUDED.total_count,
            updated_at = CURRENT_TIMESTAMP
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":year_start", score.yearStart);
    query.bindValue(":year", score.year);
    query.bindValue(":earned", score.earnedScore);
    query.bindValue(":target", score.targetScore);
    query.bindValue(":percentage", score.completionPercentage);
    query.bindValue(":completed", score.completedCount);
    query.bindValue(":skipped", score.skippedCount);
    query.bindValue(":not_completed", score.notCompletedCount);
    query.bindValue(":pending", score.pendingCount);
    query.bindValue(":total", score.totalCount);

    return query.exec();
}

DailyScore* ScoreRepository::getDailyScore(const QDate& date)
{
    QString sql = "SELECT * FROM daily_scores WHERE date = :date";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":date", date);

    if (!query.exec() || !query.next()) {
        return nullptr;
    }

    return mapDailyFromRecord(query.record());
}

WeeklyScore* ScoreRepository::getWeeklyScore(const QDate& weekStart)
{
    QString sql = "SELECT * FROM weekly_scores WHERE week_start = :week_start";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":week_start", weekStart);

    if (!query.exec() || !query.next()) {
        return nullptr;
    }

    return mapWeeklyFromRecord(query.record());
}

MonthlyScore* ScoreRepository::getMonthlyScore(const QDate& monthStart)
{
    QString sql = "SELECT * FROM monthly_scores WHERE month_start = :month_start";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":month_start", monthStart);

    if (!query.exec() || !query.next()) {
        return nullptr;
    }

    return mapMonthlyFromRecord(query.record());
}

YearlyScore* ScoreRepository::getYearlyScore(int year)
{
    QDate yearStart(year, 1, 1);
    QString sql = "SELECT * FROM yearly_scores WHERE year_start = :year_start";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":year_start", yearStart);

    if (!query.exec() || !query.next()) {
        return nullptr;
    }

    return mapYearlyFromRecord(query.record());
}

QList<DailyScore*> ScoreRepository::getDailyScoreRange(const QDate& start, const QDate& end)
{
    QString sql = "SELECT * FROM daily_scores WHERE date >= :start AND date <= :end ORDER BY date DESC";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":start", start);
    query.bindValue(":end", end);

    QList<DailyScore*> scores;
    if (query.exec()) {
        while (query.next()) {
            scores.append(mapDailyFromRecord(query.record()));
        }
    }

    return scores;
}

QList<WeeklyScore*> ScoreRepository::getWeeklyScoreRange(int weekCount)
{
    QString sql = "SELECT * FROM weekly_scores ORDER BY week_start DESC LIMIT :limit";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":limit", weekCount);

    QList<WeeklyScore*> scores;
    if (query.exec()) {
        while (query.next()) {
            scores.append(mapWeeklyFromRecord(query.record()));
        }
    }

    return scores;
}

QList<MonthlyScore*> ScoreRepository::getMonthlyScoreRange(int monthCount)
{
    QString sql = "SELECT * FROM monthly_scores ORDER BY month_start DESC LIMIT :limit";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":limit", monthCount);

    QList<MonthlyScore*> scores;
    if (query.exec()) {
        while (query.next()) {
            scores.append(mapMonthlyFromRecord(query.record()));
        }
    }

    return scores;
}

DailyScore* ScoreRepository::mapDailyFromRecord(const QSqlRecord& record)
{
    DailyScore* score = new DailyScore();
    score->date = record.value("date").toDate();
    score->earnedScore = record.value("earned_score").toInt();
    score->targetScore = record.value("target_score").toInt();
    score->completionPercentage = record.value("completion_percentage").toDouble();
    score->completedCount = record.value("completed_count").toInt();
    score->skippedCount = record.value("skipped_count").toInt();
    score->notCompletedCount = record.value("not_completed_count").toInt();
    score->pendingCount = record.value("pending_count").toInt();
    score->totalCount = record.value("total_count").toInt();
    score->perfectDay = record.value("perfect_day").toBool();
    score->hasNegativeOutcome = record.value("has_negative_outcome").toBool();

    return score;
}

WeeklyScore* ScoreRepository::mapWeeklyFromRecord(const QSqlRecord& record)
{
    WeeklyScore* score = new WeeklyScore();
    score->weekStart = record.value("week_start").toDate();
    score->year = record.value("year").toInt();
    score->weekNumber = record.value("week_number").toInt();
    score->earnedScore = record.value("earned_score").toInt();
    score->targetScore = record.value("target_score").toInt();
    score->completionPercentage = record.value("completion_percentage").toDouble();
    score->completedCount = record.value("completed_count").toInt();
    score->skippedCount = record.value("skipped_count").toInt();
    score->notCompletedCount = record.value("not_completed_count").toInt();
    score->pendingCount = record.value("pending_count").toInt();
    score->totalCount = record.value("total_count").toInt();

    return score;
}

MonthlyScore* ScoreRepository::mapMonthlyFromRecord(const QSqlRecord& record)
{
    MonthlyScore* score = new MonthlyScore();
    score->monthStart = record.value("month_start").toDate();
    score->year = record.value("year").toInt();
    score->month = record.value("month").toInt();
    score->earnedScore = record.value("earned_score").toInt();
    score->targetScore = record.value("target_score").toInt();
    score->completionPercentage = record.value("completion_percentage").toDouble();
    score->completedCount = record.value("completed_count").toInt();
    score->skippedCount = record.value("skipped_count").toInt();
    score->notCompletedCount = record.value("not_completed_count").toInt();
    score->pendingCount = record.value("pending_count").toInt();
    score->totalCount = record.value("total_count").toInt();

    return score;
}

YearlyScore* ScoreRepository::mapYearlyFromRecord(const QSqlRecord& record)
{
    YearlyScore* score = new YearlyScore();
    score->yearStart = record.value("year_start").toDate();
    score->year = record.value("year").toInt();
    score->earnedScore = record.value("earned_score").toInt();
    score->targetScore = record.value("target_score").toInt();
    score->completionPercentage = record.value("completion_percentage").toDouble();
    score->completedCount = record.value("completed_count").toInt();
    score->skippedCount = record.value("skipped_count").toInt();
    score->notCompletedCount = record.value("not_completed_count").toInt();
    score->pendingCount = record.value("pending_count").toInt();
    score->totalCount = record.value("total_count").toInt();

    return score;
}
