#ifndef STREAKSERVICE_H
#define STREAKSERVICE_H

#include <QObject>
#include <QDate>
#include "repositories/streakrepository.h"
#include "repositories/scorerepository.h"

class StreakService : public QObject
{
    Q_OBJECT

public:
    explicit StreakService(StreakRepository* streakRepo,
                           ScoreRepository* scoreRepo,
                           QObject *parent = nullptr);

    // Streak updates
    void updateStreaksForDate(const QDate& date);
    void updateStreakForGoal(const QString& goalId, const QDate& date);

    // Streak queries
    Streak* getDailyStreak();
    Streak* getWeeklyStreak();
    Streak* getMonthlyStreak();
    Streak* getYearlyStreak();
    Streak* getGoalStreak(const QString& goalId, const QString& scope);

signals:
    void streakUpdated(const QString& streakId);
    void streakBroken(const QString& scope, const QDate& date);

private:
    bool shouldBreakStreak(double completionPercentage);
    bool hasNegativeOutcome(const QDate& date);

    StreakRepository* m_streakRepo;
    ScoreRepository* m_scoreRepo;
};

#endif // STREAKSERVICE_H
