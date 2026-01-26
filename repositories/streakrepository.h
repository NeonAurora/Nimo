#ifndef STREAKREPOSITORY_H
#define STREAKREPOSITORY_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QDate>

struct Streak {
    QString id;
    QString goalId;  // NULL for overall streak
    QString scope;
    int currentStreak;
    int longestStreak;
    QDate lastSuccessDate;
    QDate lastBreakDate;
    int totalSuccesses;
    int totalFailures;
    double successRate;
};

class StreakRepository : public QObject
{
    Q_OBJECT

public:
    explicit StreakRepository(QSqlDatabase db, QObject *parent = nullptr);

    // CRUD
    Streak* create(const Streak& streak);
    Streak* findById(const QString& id);
    Streak* findByGoalAndScope(const QString& goalId, const QString& scope);
    Streak* findOverallByScope(const QString& scope);
    bool update(const Streak& streak);

    // Get or create
    Streak* getOrCreate(const QString& goalId, const QString& scope);
    Streak* getOrCreateOverall(const QString& scope);

private:
    Streak* mapFromRecord(const QSqlRecord& record);

    QSqlDatabase m_db;
};

#endif // STREAKREPOSITORY_H
