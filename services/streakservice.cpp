#include "services/streakservice.h"
#include "logging/logger.h"
#include "logging/requestscope.h"

StreakService::StreakService(StreakRepository* streakRepo,
                             ScoreRepository* scoreRepo,
                             QObject *parent)
    : QObject(parent)
    , m_streakRepo(streakRepo)
    , m_scoreRepo(scoreRepo)
{
}

void StreakService::updateStreaksForDate(const QDate& date)
{
    RequestScope scope("StreakService::updateStreaksForDate", "UPDATE", {
                                                                            {"date", date.toString("yyyy-MM-dd")}
                                                                        });

    // Get daily score
    DailyScore* dailyScore = m_scoreRepo->getDailyScore(date);
    if (!dailyScore) {
        scope.logError("Daily score not found", "NOT_FOUND");
        return;
    }

    // Get overall daily streak
    Streak* streak = m_streakRepo->getOrCreateOverall("daily");
    if (!streak) {
        scope.logError("Failed to get or create streak", "STREAK_ERROR");
        delete dailyScore;
        return;
    }

    // Check if streak should break
    if (shouldBreakStreak(dailyScore->completionPercentage) ||
        dailyScore->hasNegativeOutcome) {
        // Break streak
        streak->currentStreak = 0;
        streak->lastBreakDate = date;
        streak->totalFailures++;
        emit streakBroken("daily", date);
    } else {
        // Continue streak
        streak->currentStreak++;
        streak->lastSuccessDate = date;
        streak->totalSuccesses++;

        // Update longest streak if needed
        if (streak->currentStreak > streak->longestStreak) {
            streak->longestStreak = streak->currentStreak;
        }
    }

    // Update success rate
    int total = streak->totalSuccesses + streak->totalFailures;
    if (total > 0) {
        streak->successRate = (static_cast<double>(streak->totalSuccesses) / total) * 100.0;
    }

    // Save streak
    if (m_streakRepo->update(*streak)) {
        emit streakUpdated(streak->id);
        scope.logSuccess({
            {"currentStreak", streak->currentStreak},
            {"longestStreak", streak->longestStreak}
        });
    } else {
        scope.logError("Failed to update streak", "UPDATE_FAILED");
    }

    delete dailyScore;
    delete streak;
}

void StreakService::updateStreakForGoal(const QString& goalId, const QDate& date)
{
    // Similar logic for goal-specific streaks
    // Implementation can be added later
}

Streak* StreakService::getDailyStreak()
{
    return m_streakRepo->getOrCreateOverall("daily");
}

Streak* StreakService::getWeeklyStreak()
{
    return m_streakRepo->getOrCreateOverall("weekly");
}

Streak* StreakService::getMonthlyStreak()
{
    return m_streakRepo->getOrCreateOverall("monthly");
}

Streak* StreakService::getYearlyStreak()
{
    return m_streakRepo->getOrCreateOverall("yearly");
}

Streak* StreakService::getGoalStreak(const QString& goalId, const QString& scope)
{
    return m_streakRepo->getOrCreate(goalId, scope);
}

bool StreakService::shouldBreakStreak(double completionPercentage)
{
    // Break streak if completion is below 80%
    return completionPercentage < 80.0;
}

bool StreakService::hasNegativeOutcome(const QDate& date)
{
    DailyScore* score = m_scoreRepo->getDailyScore(date);
    if (!score) {
        return false;
    }

    bool hasNegative = score->hasNegativeOutcome;
    delete score;
    return hasNegative;
}
