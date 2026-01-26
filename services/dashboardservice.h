#ifndef DASHBOARDSERVICE_H
#define DASHBOARDSERVICE_H

#include <QObject>
#include <QDate>
#include "repositories/scorerepository.h"
#include "repositories/streakrepository.h"

struct DashboardData {
    DailyScore* today;
    WeeklyScore* thisWeek;
    MonthlyScore* thisMonth;
    YearlyScore* thisYear;

    Streak* dailyStreak;
    Streak* weeklyStreak;
    Streak* monthlyStreak;
    Streak* yearlyStreak;

    QList<DailyScore*> dailyTrend;
    QList<WeeklyScore*> weeklyTrend;
    QList<MonthlyScore*> monthlyTrend;
};

class DashboardService : public QObject
{
    Q_OBJECT

public:
    explicit DashboardService(ScoreRepository* scoreRepo,
                              StreakRepository* streakRepo,
                              QObject *parent = nullptr);

    Q_INVOKABLE void refreshDashboard();
    DashboardData* data() const { return m_data; }

signals:
    void dataChanged();
    void dashboardReady();

private:
    ScoreRepository* m_scoreRepo;
    StreakRepository* m_streakRepo;
    DashboardData* m_data;
};

#endif // DASHBOARDSERVICE_H
