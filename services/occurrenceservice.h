#ifndef OCCURRENCESERVICE_H
#define OCCURRENCESERVICE_H

#include <QObject>
#include <QString>
#include <QDate>
#include <QList>
#include "repositories/occurrencerepository.h"

class OccurrenceService : public QObject
{
    Q_OBJECT

public:
    explicit OccurrenceService(OccurrenceRepository* occurrenceRepo, QObject *parent = nullptr);

    // Status management
    bool markCompleted(const QString& occurrenceId);
    bool markSkipped(const QString& occurrenceId);
    bool markNotCompleted(const QString& occurrenceId);
    bool setStatus(const QString& occurrenceId, const QString& status);

    // Queries
    QList<Occurrence*> getOccurrencesForDate(const QDate& date);
    QList<Occurrence*> getOccurrencesForWeek(const QDate& date);
    QList<Occurrence*> getOccurrencesForMonth(const QDate& date);
    QList<Occurrence*> getOccurrencesForYear(const QDate& date);

    // Ensure occurrences exist
    void ensureOccurrencesExist(const QDate& date, const QList<Goal*>& goals);

signals:
    void occurrenceUpdated(Occurrence* occurrence);
    void scoresNeedRecalculation(const QDate& date);

private:
    OccurrenceRepository* m_occurrenceRepo;
};

#endif // OCCURRENCESERVICE_H
