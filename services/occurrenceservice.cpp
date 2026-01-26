#include "services/occurrenceservice.h"
#include "logging/logger.h"
#include "logging/requestscope.h"

OccurrenceService::OccurrenceService(OccurrenceRepository* occurrenceRepo, QObject *parent)
    : QObject(parent)
    , m_occurrenceRepo(occurrenceRepo)
{
    connect(m_occurrenceRepo, &OccurrenceRepository::occurrenceStatusChanged,
            this, [this](const QString& occurrenceId) {
                Occurrence* occurrence = m_occurrenceRepo->findById(occurrenceId);
                if (occurrence) {
                    emit occurrenceUpdated(occurrence);

                    // Trigger score recalculation
                    if (occurrence->date.isValid()) {
                        emit scoresNeedRecalculation(occurrence->date);
                    }
                }
            });
}

bool OccurrenceService::markCompleted(const QString& occurrenceId)
{
    return setStatus(occurrenceId, "completed");
}

bool OccurrenceService::markSkipped(const QString& occurrenceId)
{
    return setStatus(occurrenceId, "skipped");
}

bool OccurrenceService::markNotCompleted(const QString& occurrenceId)
{
    return setStatus(occurrenceId, "not_completed");
}

bool OccurrenceService::setStatus(const QString& occurrenceId, const QString& status)
{
    RequestScope scope("OccurrenceService::setStatus", "UPDATE", {
                                                                     {"occurrenceId", occurrenceId},
                                                                     {"status", status}
                                                                 });

    // Validate status
    QStringList validStatuses = {"pending", "completed", "skipped", "not_completed"};
    if (!validStatuses.contains(status)) {
        scope.logError("Invalid status", "VALIDATION_FAILED");
        return false;
    }

    // Update status
    bool success = m_occurrenceRepo->updateStatus(occurrenceId, status);

    if (!success) {
        scope.logError("Failed to update occurrence status", "UPDATE_FAILED");
        return false;
    }

    scope.logSuccess({
        {"occurrenceId", occurrenceId},
        {"newStatus", status}
    });

    return true;
}

QList<Occurrence*> OccurrenceService::getOccurrencesForDate(const QDate& date)
{
    return m_occurrenceRepo->findByDate(date);
}

QList<Occurrence*> OccurrenceService::getOccurrencesForWeek(const QDate& date)
{
    QDate weekStart = m_occurrenceRepo->calculateWeekStart(date);
    return m_occurrenceRepo->findByWeek(weekStart);
}

QList<Occurrence*> OccurrenceService::getOccurrencesForMonth(const QDate& date)
{
    QDate monthStart = m_occurrenceRepo->calculateMonthStart(date);
    return m_occurrenceRepo->findByMonth(monthStart);
}

QList<Occurrence*> OccurrenceService::getOccurrencesForYear(const QDate& date)
{
    return m_occurrenceRepo->findByYear(date.year());
}

void OccurrenceService::ensureOccurrencesExist(const QDate& date, const QList<Goal*>& goals)
{
    RequestScope scope("OccurrenceService::ensureOccurrencesExist", "CREATE", {
                                                                                  {"date", date.toString("yyyy-MM-dd")},
                                                                                  {"goalCount", goals.size()}
                                                                              });

    int created = 0;
    for (Goal* goal : goals) {
        Occurrence* existing = m_occurrenceRepo->getOrCreate(goal->id, date, goal->scope);
        if (existing) {
            created++;
        }
    }

    scope.logSuccess({{"created", created}});
}
