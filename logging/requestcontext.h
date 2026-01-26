#ifndef REQUESTCONTEXT_H
#define REQUESTCONTEXT_H

#include <QString>
#include <QAtomicInteger>

class RequestContext
{
public:
    // Generate a new unique request ID
    static QString generate();

    // Get/Set current thread-local request ID
    static QString current();
    static void setCurrent(const QString& requestId);
    static void clear();

private:
    static thread_local QString s_currentRequestId;
    static QAtomicInteger<quint32> s_counter;
};

#endif // REQUESTCONTEXT_H
