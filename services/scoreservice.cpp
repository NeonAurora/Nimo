#include "services/scoreservice.h"
#include "logging/logger.h"
#include "logging/requestscope.h"

ScoreService::ScoreService(ScoreRepository* scoreRepo,
                          OccurrenceRepository* occurrenceRepo,
                          GoalRepository* goalRepo,
                          QObject *parent)
    : QObject(parent)
    , m_scoreRepo(scoreRepo)
    , m_occurrenceRepo(occurrenceRepo)
    , m_goalRepo(goalRepo)
{
}

void ScoreService::recalculateDaily(const QDate& date)
{
    RequestScope scope("ScoreService::recalculateDaily", "CALCULATE", {
        {"date", date.toString("yyyy-MM-dd")}
    });

    // Get daily occurrences
    QList<Occurrence*> occurrences = m_occurrenceRepo->findByDate(date);

    // Get daily goals
    QList<Goal*> goals = m_goalRepo->findByScope("daily");

    // Calculate scores
    ScoreCalculation calc = calculateFromOccurrences(occurrences, goals);

    // Create score object
    DailyScore score;
    score.date = date;
    score.earnedScore = calc.earnedScore;
    score.targetScore = calc.targetScore;
    score.completionPercentage = calc.completionPercentage;
    score.completedCount = calc.completedCount;
    score.skippedCount = calc.skippedCount;
    score.notCompletedCount = calc.notCompletedCount;
    score.pendingCount = calc.pendingCount;
    score.totalCount = calc.totalCount;
    score.perfectDay = (calc.completedCount == calc.totalCount && calc.totalCount > 0);
    score.hasNegativeOutcome = calc.hasNegativeOutcome;

    // Save to database
    if (m_scoreRepo->upsertDailyScore(score)) {
        scope.logSuccess({
            {"earnedScore", score.earnedScore},
            {"targetScore", score.targetScore},
            {"completion", score.completionPercentage}
        });
        emit dailyScoreUpdated(date);
    } else {
        scope.logError("Failed to save daily score", "SAVE_FAILED");
    }

    qDeleteAll(occurrences);
    qDeleteAll(goals);
}

void ScoreService::recalculateWeekly(const QDate& date)
{
    QDate weekStart = m_occurrenceRepo->calculateWeekStart(date);

    QList<Occurrence*> occurrences = m_occurrenceRepo->findByWeek(weekStart);
    QList<Goal*> goals = m_goalRepo->findByScope("weekly");

    ScoreCalculation calc = calculateFromOccurrences(occurrences, goals);

    WeeklyScore score;
    score.weekStart = weekStart;
    score.year = weekStart.year();
    score.weekNumber = weekStart.weekNumber();
    score.earnedScore = calc.earnedScore;
    score.targetScore = calc.targetScore;
    score.completionPercentage = calc.completionPercentage;
    score.completedCount = calc.completedCount;
    score.skippedCount = calc.skippedCount;
    score.notCompletedCount = calc.notCompletedCount;
    score.pendingCount = calc.pendingCount;
    score.totalCount = calc.totalCount;

    if (m_scoreRepo->upsertWeeklyScore(score)) {
        emit weeklyScoreUpdated(weekStart);
    }

    qDeleteAll(occurrences);
    qDeleteAll(goals);
}

void ScoreService::recalculateMonthly(const QDate& date)
{
    QDate monthStart = m_occurrenceRepo->calculateMonthStart(date);

    QList<Occurrence*> occurrences = m_occurrenceRepo->findByMonth(monthStart);
    QList<Goal*> goals = m_goalRepo->findByScope("monthly");

    ScoreCalculation calc = calculateFromOccurrences(occurrences, goals);

    MonthlyScore score;
    score.monthStart = monthStart;
    score.year = monthStart.year();
    score.month = monthStart.month();
    score.earnedScore = calc.earnedScore;
    score.targetScore = calc.targetScore;
    score.completionPercentage = calc.completionPercentage;
    score.completedCount = calc.completedCount;
    score.skippedCount = calc.skippedCount;
    score.notCompletedCount = calc.notCompletedCount;
    score.pendingCount = calc.pendingCount;
    score.totalCount = calc.totalCount;

    if (m_scoreRepo->upsertMonthlyScore(score)) {
        emit monthlyScoreUpdated(monthStart);
    }

    qDeleteAll(occurrences);
    qDeleteAll(goals);
}

void ScoreService::recalculateYearly(int year)
{
    QList<Occurrence*> occurrences = m_occurrenceRepo->findByYear(year);
    QList<Goal*> goals = m_goalRepo->findByScope("yearly");

    ScoreCalculation calc = calculateFromOccurrences(occurrences, goals);

    YearlyScore score;
    score.yearStart = QDate(year, 1, 1);
    score.year = year;
    score.earnedScore = calc.earnedScore;
    score.targetScore = calc.targetScore;
    score.completionPercentage = calc.completionPercentage;
    score.completedCount = calc.completedCount;
    score.skippedCount = calc.skippedCount;
    score.notCompletedCount = calc.notCompletedCount;
    score.pendingCount = calc.pendingCount;
    score.totalCount = calc.totalCount;

    if (m_scoreRepo->upsertYearlyScore(score)) {
        emit yearlyScoreUpdated(year);
    }

    qDeleteAll(occurrences);
    qDeleteAll(goals);
}

DailyScore* ScoreService::getDailyScore(const QDate& date)
{
    return m_scoreRepo->getDailyScore(date);
}

WeeklyScore* ScoreService::getWeeklyScore(const QDate& date)
{
    QDate weekStart = m_occurrenceRepo->calculateWeekStart(date);
    return m_scoreRepo->getWeeklyScore(weekStart);
}

MonthlyScore* ScoreService::getMonthlyScore(const QDate& date)
{
    QDate monthStart = m_occurrenceRepo->calculateMonthStart(date);
    return m_scoreRepo->getMonthlyScore(monthStart);
}

YearlyScore* ScoreService::getYearlyScore(int year)
{
    return m_scoreRepo->getYearlyScore(year);
}

QList<DailyScore*> ScoreService::getDailyTrend(int days)
{
    QDate end = QDate::currentDate();
    QDate start = end.addDays(-days + 1);
    return m_scoreRepo->getDailyScoreRange(start, end);
}

QList<WeeklyScore*> ScoreService::getWeeklyTrend(int weeks)
{
    return m_scoreRepo->getWeeklyScoreRange(weeks);
}

QList<MonthlyScore*> ScoreService::getMonthlyTrend(int months)
{
    return m_scoreRepo->getMonthlyScoreRange(months);
}

ScoreService::ScoreCalculation ScoreService::calculateFromOccurrences(
    const QList<Occurrence*>& occurrences,
    const QList<Goal*>& goals)
{
    ScoreCalculation calc;
    calc.earnedScore = 0;
    calc.targetScore = 0;
    calc.completionPercentage = 0.0;
    calc.completedCount = 0;
    calc.skippedCount = 0;
    calc.notCompletedCount = 0;
    calc.pendingCount = 0;
    calc.totalCount = occurrences.size();
    calc.hasNegativeOutcome = false;

    // Calculate target score from goals
    for (Goal* goal : goals) {
        if (goal->points > 0) {
            calc.targetScore += goal->points;
        }
    }

    // Calculate earned score from occurrences
    for (Occurrence* occurrence : occurrences) {
        calc.earnedScore += occurrence->scoreImpact;

        if (occurrence->status == "completed") {
            calc.completedCount++;
        } else if (occurrence->status == "skipped") {
            calc.skippedCount++;
        } else if (occurrence->status == "not_completed") {
            calc.notCompletedCount++;
            if (occurrence->scoreImpact < 0) {
                calc.hasNegativeOutcome = true;
            }
        } else if (occurrence->status == "pending") {
            calc.pendingCount++;
        }
    }

    // Calculate completion percentage
    if (calc.targetScore > 0) {
        calc.completionPercentage = (static_cast<double>(calc.earnedScore) / calc.targetScore) * 100.0;
    }

    return calc;
}
