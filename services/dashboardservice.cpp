#include "services/dashboardservice.h"
#include "logging/logger.h"
#include "logging/requestscope.h"

DashboardService::DashboardService(ScoreRepository* scoreRepo,
                                   StreakRepository* streakRepo,
                                   QObject *parent)
    : QObject(parent)
    , m_scoreRepo(scoreRepo)
    , m_streakRepo(streakRepo)
    , m_data(new DashboardData())
{
}

void DashboardService::refreshDashboard()
{
    RequestScope scope("DashboardService::refreshDashboard", "READ", {});

    QDate today = QDate::currentDate();

    // Current scores
    m_data->today = m_scoreRepo->getDailyScore(today);

    QDate weekStart = today;
    while (weekStart.dayOfWeek() != Qt::Monday) {
        weekStart = weekStart.addDays(-1);
    }
    m_data->thisWeek = m_scoreRepo->getWeeklyScore(weekStart);

    QDate monthStart(today.year(), today.month(), 1);
    m_data->thisMonth = m_scoreRepo->getMonthlyScore(monthStart);

    m_data->thisYear = m_scoreRepo->getYearlyScore(today.year());

    // Streaks
    m_data->dailyStreak = m_streakRepo->findOverallByScope("daily");
    m_data->weeklyStreak = m_streakRepo->findOverallByScope("weekly");
    m_data->monthlyStreak = m_streakRepo->findOverallByScope("monthly");
    m_data->yearlyStreak = m_streakRepo->findOverallByScope("yearly");

    // Trends
    m_data->dailyTrend = m_scoreRepo->getDailyScoreRange(today.addDays(-30), today);
    m_data->weeklyTrend = m_scoreRepo->getWeeklyScoreRange(12);
    m_data->monthlyTrend = m_scoreRepo->getMonthlyScoreRange(12);

    scope.logSuccess({
        {"trendsLoaded", true}
    });

    emit dataChanged();
    emit dashboardReady();
}
