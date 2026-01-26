#ifndef OCCURRENCEREPOSITORY_H
#define OCCURRENCEREPOSITORY_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QList>
#include <QDate>

struct Occurrence {
    QString id;
    QString goalId;
    QDate date;
    QDate weekStart;
    QDate monthStart;
    QDate yearStart;
    QString status;  // pending, completed, skipped, not_completed
    QDateTime completedAt;
    int scoreImpact;
    QString notes;
};

class OccurrenceRepository : public QObject
{
    Q_OBJECT

public:
    explicit OccurrenceRepository(QSqlDatabase db, QObject *parent = nullptr);

    // CRUD
    Occurrence* create(const Occurrence& occurrence);
    Occurrence* findById(const QString& id);
    bool update(const Occurrence& occurrence);
    bool updateStatus(const QString& id, const QString& status);

    // Queries by time window
    QList<Occurrence*> findByDate(const QDate& date);
    QList<Occurrence*> findByWeek(const QDate& weekStart);
    QList<Occurrence*> findByMonth(const QDate& monthStart);
    QList<Occurrence*> findByYear(int year);

    // Get or create
    Occurrence* getOrCreate(const QString& goalId, const QDate& date, const QString& scope);

    // Batch operations
    void generateOccurrencesForDate(const QDate& date, const QList<QString>& goalIds);

signals:
    void occurrenceStatusChanged(const QString& occurrenceId);

private:
    Occurrence* mapFromRecord(const QSqlRecord& record);
    QDate calculateWeekStart(const QDate& date);
    QDate calculateMonthStart(const QDate& date);
    QDate calculateYearStart(const QDate& date);

    QSqlDatabase m_db;
};

#endif // OCCURRENCEREPOSITORY_H
