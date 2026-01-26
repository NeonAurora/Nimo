#include "logging/requestcontext.h"
#include <QDateTime>
#include <QRandomGenerator>

thread_local QString RequestContext::s_currentRequestId;
QAtomicInteger<quint32> RequestContext::s_counter(0);

QString RequestContext::generate()
{
    // Generate format: req_xxxxx (5 random alphanumeric chars)
    quint32 counter = s_counter.fetchAndAddOrdered(1);
    quint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    quint32 random = QRandomGenerator::global()->generate();

    // Combine timestamp, counter, and random for uniqueness
    quint64 combined = (timestamp & 0xFFFFFFFF) ^ (counter << 16) ^ random;

    // Convert to base36 for shorter string
    QString id = QString::number(combined, 36).right(5).toUpper();

    return QString("req_%1").arg(id);
}

QString RequestContext::current()
{
    return s_currentRequestId;
}

void RequestContext::setCurrent(const QString& requestId)
{
    s_currentRequestId = requestId;
}

void RequestContext::clear()
{
    s_currentRequestId.clear();
}
