#ifndef SCOREREPOSITORY_H
#define SCOREREPOSITORY_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QDate>
#include <QList>

struct DailyScore {
    QDate date;
    int earnedScore;
    int targetScore;
    double completionPercentage;
    int completedCount;
    int skippedCount;
    int notCompletedCount;
    int pendingCount;
    int totalCount;
    bool perfectDay;
    bool hasNegativeOutcome;
};

struct WeeklyScore {
    QDate weekStart;
    int year;
    int weekNumber;
    int earnedScore;
    int targetScore;
    double completionPercentage;
    int completedCount;
    int skippedCount;
    int notCompletedCount;
    int pendingCount;
    int totalCount;
};

struct MonthlyScore {
    QDate monthStart;
    int year;
    int month;
    int earnedScore;
    int targetScore;
    double completionPercentage;
    int completedCount;
    int skippedCount;
    int notCompletedCount;
    int pendingCount;
    int totalCount;
};

struct YearlyScore {
    QDate yearStart;
    int year;
    int earnedScore;
    int targetScore;
    double completionPercentage;
    int completedCount;
    int skippedCount;
    int notCompletedCount;
    int pendingCount;
    int totalCount;
};

class ScoreRepository : public QObject
{
    Q_OBJECT

public:
    explicit ScoreRepository(QSqlDatabase db, QObject *parent = nullptr);

    // Upsert scores
    bool upsertDailyScore(const DailyScore& score);
    bool upsertWeeklyScore(const WeeklyScore& score);
    bool upsertMonthlyScore(const MonthlyScore& score);
    bool upsertYearlyScore(const YearlyScore& score);

    // Fetch scores
    DailyScore* getDailyScore(const QDate& date);
    WeeklyScore* getWeeklyScore(const QDate& weekStart);
    MonthlyScore* getMonthlyScore(const QDate& monthStart);
    YearlyScore* getYearlyScore(int year);

    // Range queries for charts
    QList<DailyScore*> getDailyScoreRange(const QDate& start, const QDate& end);
    QList<WeeklyScore*> getWeeklyScoreRange(int weekCount);
    QList<MonthlyScore*> getMonthlyScoreRange(int monthCount);

private:
    DailyScore* mapDailyFromRecord(const QSqlRecord& record);
    WeeklyScore* mapWeeklyFromRecord(const QSqlRecord& record);
    MonthlyScore* mapMonthlyFromRecord(const QSqlRecord& record);
    YearlyScore* mapYearlyFromRecord(const QSqlRecord& record);

    QSqlDatabase m_db;
};

#endif // SCOREREPOSITORY_H
