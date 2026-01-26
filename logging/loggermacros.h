#ifndef LOGGERMACROS_H
#define LOGGERMACROS_H

#include "logger.h"
#include "requestcontext.h"

// Helper to get source location
#define LOG_SOURCE QString("%1::%2").arg(__FILE__).arg(__FUNCTION__)

// Basic logging macros
#define LOG_DEBUG(contextId, message, ...) \
Logger::instance().debug(LOG_SOURCE, contextId, message, ##__VA_ARGS__)

#define LOG_INFO(contextId, message, ...) \
    Logger::instance().info(LOG_SOURCE, contextId, message, ##__VA_ARGS__)

#define LOG_WARN(contextId, message, ...) \
    Logger::instance().warn(LOG_SOURCE, contextId, message, ##__VA_ARGS__)

#define LOG_ERROR(contextId, message, ...) \
    Logger::instance().error(LOG_SOURCE, contextId, message, ##__VA_ARGS__)

#define LOG_FATAL(contextId, message, ...) \
    Logger::instance().fatal(LOG_SOURCE, contextId, message, ##__VA_ARGS__)

// Request/Response macros
#define LOG_REQUEST(requestId, operation, entity, params) \
    Logger::instance().logRequest(LOG_SOURCE, requestId, operation, entity, params)

#define LOG_RESPONSE(requestId, operation, entity, duration, success, result) \
    Logger::instance().logResponse(LOG_SOURCE, requestId, operation, entity, \
                     duration, success, result)

#define LOG_ERROR_DETAILS(requestId, operation, entity, errorMsg, errorCode, duration, stack) \
    Logger::instance().logError(LOG_SOURCE, requestId, operation, entity, \
                  errorMsg, errorCode, duration, stack)

#define LOG_QUERY(requestId, sql, bindValues) \
    Logger::instance().logQuery(LOG_SOURCE, requestId, sql, bindValues)

// Transaction macros
#define LOG_TXN_START(transactionId, initiatedBy, requestId) \
    Logger::instance().logTransactionStart(transactionId, initiatedBy, requestId)

#define LOG_TXN_END(transactionId, requestId, duration, committed, operations) \
    Logger::instance().logTransactionEnd(transactionId, requestId, duration, \
                           committed, operations)

#endif // LOGGERMACROS_H
