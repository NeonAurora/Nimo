#include "services/goalservice.h"
#include "logging/logger.h"
#include "logging/requestscope.h"
#include <QRegularExpression>

GoalService::GoalService(GoalRepository* goalRepo, QObject *parent)
    : QObject(parent)
    , m_goalRepo(goalRepo)
{
    // Connect repository signals
    connect(m_goalRepo, &GoalRepository::goalCreated,
            this, [this](const QString& goalId) {
                Goal* goal = m_goalRepo->findById(goalId);
                if (goal) {
                    emit goalCreated(goal);
                }
            });

    connect(m_goalRepo, &GoalRepository::goalUpdated,
            this, [this](const QString& goalId) {
                Goal* goal = m_goalRepo->findById(goalId);
                if (goal) {
                    emit goalUpdated(goal);
                }
            });

    connect(m_goalRepo, &GoalRepository::goalDeleted,
            this, &GoalService::goalDeleted);
}

Goal* GoalService::createGoal(const QString& title,
                              const QString& scope,
                              int points,
                              const QString& missingBehavior,
                              int penaltyPoints)
{
    RequestScope reqScope("GoalService::createGoal", "CREATE", {
                                                                   {"title", title},
                                                                   {"scope", scope},
                                                                   {"points", points}
                                                               });

    // Create goal with default values
    Goal goal;
    goal.title = title;
    goal.scope = scope;
    goal.points = points;
    goal.missingBehavior = missingBehavior;
    goal.penaltyPoints = penaltyPoints;
    goal.category = "";
    goal.notes = "";
    goal.iconName = "";
    goal.colorHex = "";
    goal.sortOrder = 0;
    goal.isActive = true;

    // Validate
    QString errorMessage;
    if (!validateGoal(goal, errorMessage)) {
        reqScope.logError(errorMessage, "VALIDATION_FAILED");
        emit errorOccurred(errorMessage);
        return nullptr;
    }

    // Create in repository
    Goal* created = m_goalRepo->create(goal);

    if (!created) {
        reqScope.logError("Failed to create goal in repository", "CREATE_FAILED");
        emit errorOccurred("Failed to create goal");
        return nullptr;
    }

    reqScope.logSuccess({
        {"goalId", created->id},
        {"title", created->title}
    });

    return created;
}

Goal* GoalService::createGoalFull(const QString& title,
                                  const QString& scope,
                                  int points,
                                  const QString& missingBehavior,
                                  int penaltyPoints,
                                  const QString& category,
                                  const QString& notes,
                                  const QString& iconName,
                                  const QString& colorHex,
                                  int sortOrder)
{
    RequestScope reqScope("GoalService::createGoalFull", "CREATE", {
                                                                       {"title", title},
                                                                       {"scope", scope},
                                                                       {"points", points},
                                                                       {"category", category}
                                                                   });

    Goal goal;
    goal.title = title;
    goal.scope = scope;
    goal.points = points;
    goal.missingBehavior = missingBehavior;
    goal.penaltyPoints = penaltyPoints;
    goal.category = category;
    goal.notes = notes;
    goal.iconName = iconName;
    goal.colorHex = colorHex;
    goal.sortOrder = sortOrder;
    goal.isActive = true;

    // Validate
    QString errorMessage;
    if (!validateGoal(goal, errorMessage)) {
        reqScope.logError(errorMessage, "VALIDATION_FAILED");
        emit errorOccurred(errorMessage);
        return nullptr;
    }

    // Create in repository
    Goal* created = m_goalRepo->create(goal);

    if (!created) {
        reqScope.logError("Failed to create goal in repository", "CREATE_FAILED");
        emit errorOccurred("Failed to create goal");
        return nullptr;
    }

    reqScope.logSuccess({
        {"goalId", created->id},
        {"title", created->title}
    });

    return created;
}

bool GoalService::updateGoal(Goal* goal)
{
    if (!goal) {
        emit errorOccurred("Goal is null");
        return false;
    }

    RequestScope scope("GoalService::updateGoal", "UPDATE", {
                                                                {"goalId", goal->id},
                                                                {"title", goal->title}
                                                            });

    // Validate
    QString errorMessage;
    if (!validateGoal(*goal, errorMessage)) {
        scope.logError(errorMessage, "VALIDATION_FAILED");
        emit errorOccurred(errorMessage);
        return false;
    }

    // Check if goal exists
    if (!m_goalRepo->exists(goal->id)) {
        scope.logError("Goal does not exist", "NOT_FOUND");
        emit errorOccurred("Goal not found");
        return false;
    }

    // Update in repository
    bool success = m_goalRepo->update(*goal);

    if (!success) {
        scope.logError("Failed to update goal in repository", "UPDATE_FAILED");
        emit errorOccurred("Failed to update goal");
        return false;
    }

    scope.logSuccess({
        {"goalId", goal->id}
    });

    return true;
}

bool GoalService::deleteGoal(const QString& goalId)
{
    RequestScope scope("GoalService::deleteGoal", "DELETE", {
                                                                {"goalId", goalId}
                                                            });

    // Check if goal exists
    if (!m_goalRepo->exists(goalId)) {
        scope.logError("Goal does not exist", "NOT_FOUND");
        emit errorOccurred("Goal not found");
        return false;
    }

    // Soft delete
    bool success = m_goalRepo->softDelete(goalId);

    if (!success) {
        scope.logError("Failed to delete goal", "DELETE_FAILED");
        emit errorOccurred("Failed to delete goal");
        return false;
    }

    scope.logSuccess({
        {"goalId", goalId}
    });

    return true;
}

bool GoalService::toggleGoalActive(const QString& goalId)
{
    RequestScope scope("GoalService::toggleGoalActive", "UPDATE", {
                                                                      {"goalId", goalId}
                                                                  });

    Goal* goal = m_goalRepo->findById(goalId);
    if (!goal) {
        scope.logError("Goal not found", "NOT_FOUND");
        emit errorOccurred("Goal not found");
        return false;
    }

    // Toggle active state
    goal->isActive = !goal->isActive;

    bool success = m_goalRepo->update(*goal);

    if (!success) {
        scope.logError("Failed to toggle goal active state", "UPDATE_FAILED");
        delete goal;
        return false;
    }

    scope.logSuccess({
        {"goalId", goalId},
        {"isActive", goal->isActive}
    });

    delete goal;
    return true;
}

Goal* GoalService::getGoal(const QString& goalId)
{
    return m_goalRepo->findById(goalId);
}

QList<Goal*> GoalService::getAllGoals()
{
    return m_goalRepo->findAll();
}

QList<Goal*> GoalService::getGoalsByScope(const QString& scope)
{
    return m_goalRepo->findByScope(scope);
}

QList<Goal*> GoalService::getActiveGoals()
{
    return m_goalRepo->findActiveGoals();
}

bool GoalService::validateGoal(const Goal& goal, QString& errorMessage)
{
    // Validate title
    if (goal.title.trimmed().isEmpty()) {
        errorMessage = "Goal title cannot be empty";
        return false;
    }

    if (goal.title.length() > 255) {
        errorMessage = "Goal title cannot exceed 255 characters";
        return false;
    }

    // Validate scope
    QStringList validScopes = {"daily", "weekly", "monthly", "yearly"};
    if (!validScopes.contains(goal.scope)) {
        errorMessage = QString("Invalid scope: %1. Must be one of: daily, weekly, monthly, yearly")
        .arg(goal.scope);
        return false;
    }

    // Validate points
    if (goal.points < -1000 || goal.points > 1000) {
        errorMessage = "Points must be between -1000 and 1000";
        return false;
    }

    // Validate missing behavior
    QStringList validBehaviors = {"zero", "penalty"};
    if (!validBehaviors.contains(goal.missingBehavior)) {
        errorMessage = QString("Invalid missing behavior: %1. Must be 'zero' or 'penalty'")
        .arg(goal.missingBehavior);
        return false;
    }

    // Validate penalty points
    if (goal.penaltyPoints < 0 || goal.penaltyPoints > 1000) {
        errorMessage = "Penalty points must be between 0 and 1000";
        return false;
    }

    // Validate color hex format (if provided)
    if (!goal.colorHex.isEmpty()) {
        QRegularExpression colorRegex("^#[0-9A-Fa-f]{6}$");
        if (!colorRegex.match(goal.colorHex).hasMatch()) {
            errorMessage = "Invalid color format. Must be in format #RRGGBB";
            return false;
        }
    }

    return true;
}
