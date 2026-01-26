#ifndef CALENDARSERVICE_H
#define CALENDARSERVICE_H

#include <QObject>
#include <QDate>
#include <QList>
#include "repositories/scorerepository.h"

class CalendarService : public QObject
{
    Q_OBJECT

public:
    explicit CalendarService(ScoreRepository* scoreRepo, QObject *parent = nullptr);

    // Calendar data
    Q_INVOKABLE QList<DailyScore*> getMonthCalendar(int year, int month);
    Q_INVOKABLE QList<DailyScore*> getWeekCalendar(const QDate& weekStart);

    // Date helpers
    Q_INVOKABLE QDate getWeekStart(const QDate& date);
    Q_INVOKABLE QDate getMonthStart(const QDate& date);
    Q_INVOKABLE int getWeekNumber(const QDate& date);

signals:
    void calendarDataReady();

private:
    ScoreRepository* m_scoreRepo;
};

#endif // CALENDARSERVICE_H
