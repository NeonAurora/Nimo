#ifndef SCORESERVICE_H
#define SCORESERVICE_H

#include <QObject>
#include <QDate>
#include "repositories/scorerepository.h"
#include "repositories/occurrencerepository.h"
#include "repositories/goalrepository.h"

class ScoreService : public QObject
{
    Q_OBJECT

public:
    explicit ScoreService(ScoreRepository* scoreRepo,
                          OccurrenceRepository* occurrenceRepo,
                          GoalRepository* goalRepo,
                          QObject *parent = nullptr);

    // Score calculation
    void recalculateDaily(const QDate& date);
    void recalculateWeekly(const QDate& date);
    void recalculateMonthly(const QDate& date);
    void recalculateYearly(int year);

    // Score queries
    DailyScore* getDailyScore(const QDate& date);
    WeeklyScore* getWeeklyScore(const QDate& date);
    MonthlyScore* getMonthlyScore(const QDate& date);
    YearlyScore* getYearlyScore(int year);

    // Chart data
    QList<DailyScore*> getDailyTrend(int days);
    QList<WeeklyScore*> getWeeklyTrend(int weeks);
    QList<MonthlyScore*> getMonthlyTrend(int months);

signals:
    void dailyScoreUpdated(const QDate& date);
    void weeklyScoreUpdated(const QDate& weekStart);
    void monthlyScoreUpdated(const QDate& monthStart);
    void yearlyScoreUpdated(int year);

private:
    struct ScoreCalculation {
        int earnedScore;
        int targetScore;
        double completionPercentage;
        int completedCount;
        int skippedCount;
        int notCompletedCount;
        int pendingCount;
        int totalCount;
        bool hasNegativeOutcome;
    };

    ScoreCalculation calculateFromOccurrences(const QList<Occurrence*>& occurrences,
                                              const QList<Goal*>& goals);

    ScoreRepository* m_scoreRepo;
    OccurrenceRepository* m_occurrenceRepo;
    GoalRepository* m_goalRepo;
};

#endif // SCORESERVICE_H
