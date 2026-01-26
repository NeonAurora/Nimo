#ifndef REQUESTSCOPE_H
#define REQUESTSCOPE_H

#include "logging/requestcontext.h"
#include "logging/logger.h"
#include <QString>
#include <QJsonObject>
#include <QDateTime>

class RequestScope
{
public:
    explicit RequestScope(const QString& source,
                          const QString& operation,
                          const QJsonObject& params = QJsonObject())
        : m_requestId(RequestContext::generate()),
        m_source(source),
        m_operation(operation),
        m_startTime(QDateTime::currentMSecsSinceEpoch()),
        m_logged(false)
    {
        RequestContext::setCurrent(m_requestId);
        Logger::instance().logRequest(m_source, m_requestId, m_operation,
                                      QString(), params);
    }

    ~RequestScope()
    {
        qint64 duration = QDateTime::currentMSecsSinceEpoch() - m_startTime;
        if (!m_logged) {
            Logger::instance().logResponse(m_source, m_requestId, m_operation,
                                           QString(), duration, true, QJsonObject());
        }
        RequestContext::clear();
    }

    QString requestId() const { return m_requestId; }

    void logSuccess(const QJsonObject& result = QJsonObject())
    {
        qint64 duration = QDateTime::currentMSecsSinceEpoch() - m_startTime;
        Logger::instance().logResponse(m_source, m_requestId, m_operation,
                                       QString(), duration, true, result);
        m_logged = true;
    }

    void logError(const QString& errorMessage, const QString& errorCode = QString())
    {
        qint64 duration = QDateTime::currentMSecsSinceEpoch() - m_startTime;
        Logger::instance().logError(m_source, m_requestId, m_operation,
                                    QString(), errorMessage, errorCode, duration);
        m_logged = true;
    }

private:
    QString m_requestId;
    QString m_source;
    QString m_operation;
    qint64 m_startTime;
    bool m_logged;
};

#endif // REQUESTSCOPE_H
