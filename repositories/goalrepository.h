#ifndef GOALREPOSITORY_H
#define GOALREPOSITORY_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QList>
#include <QJsonObject>

// Forward declaration - Goal model will be created separately
struct Goal {
    QString id;
    QString title;
    QString scope;
    int points;
    QString missingBehavior;
    int penaltyPoints;
    QString category;
    QString notes;
    QString iconName;
    QString colorHex;
    int sortOrder;
    bool isActive;
};

class GoalRepository : public QObject
{
    Q_OBJECT

public:
    explicit GoalRepository(QSqlDatabase db, QObject *parent = nullptr);

    // CRUD operations
    Goal* create(const Goal& goal);
    Goal* findById(const QString& id);
    QList<Goal*> findAll();
    QList<Goal*> findByScope(const QString& scope);
    QList<Goal*> findActiveGoals();
    bool update(const Goal& goal);
    bool softDelete(const QString& id);
    bool hardDelete(const QString& id);

    // Queries
    int countByScope(const QString& scope);
    bool exists(const QString& id);

signals:
    void goalCreated(const QString& goalId);
    void goalUpdated(const QString& goalId);
    void goalDeleted(const QString& goalId);

private:
    Goal* mapFromRecord(const QSqlRecord& record);
    void bindGoalValues(QSqlQuery& query, const Goal& goal);

    QSqlDatabase m_db;
};

#endif // GOALREPOSITORY_H
