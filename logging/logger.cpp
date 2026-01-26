#include "logging/logger.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QRegularExpression>

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
    : QObject(nullptr)
    , m_minLevel(INFO)
    , m_consoleEnabled(true)
    , m_fileEnabled(true)
    , m_maxDaysToKeep(30)
    , m_currentLogFile(nullptr)
    , m_fileStream(nullptr)
{
    // Set default log directory: AppData/Local/Nimo/logs/
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    m_logDirectory = QDir(appDataPath).filePath("logs");
    m_currentLogDir = QDir(m_logDirectory).filePath("current");
    m_oldLogsDir = QDir(m_logDirectory).filePath("old_logs");

    // Create directories
    QDir().mkpath(m_currentLogDir);
    QDir().mkpath(m_oldLogsDir);

    // Open today's log file
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    m_lastRotationDate = today;
    QString logFilePath = QDir(m_currentLogDir).filePath(QString("nimo_%1.log").arg(today));
    openNewLogFile(logFilePath);
}

Logger::~Logger()
{
    closeCurrentLogFile();
}

void Logger::setLogLevel(Level minLevel)
{
    QMutexLocker locker(&m_mutex);
    m_minLevel = minLevel;
}

void Logger::setConsoleEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_consoleEnabled = enabled;
}

void Logger::setFileEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_fileEnabled = enabled;
}

void Logger::setLogDirectory(const QString& dir)
{
    QMutexLocker locker(&m_mutex);
    m_logDirectory = dir;
    m_currentLogDir = QDir(m_logDirectory).filePath("current");
    m_oldLogsDir = QDir(m_logDirectory).filePath("old_logs");

    QDir().mkpath(m_currentLogDir);
    QDir().mkpath(m_oldLogsDir);
}

void Logger::setMaxDaysToKeep(int days)
{
    QMutexLocker locker(&m_mutex);
    m_maxDaysToKeep = days;
}

void Logger::log(Level level, const QString& source, const QString& contextId,
                 const QString& message, const QJsonObject& metadata)
{
    QMutexLocker locker(&m_mutex);

    if (level < m_minLevel) {
        return;
    }

    checkDailyRotation();

    QString logLine = formatLogLine(level, source, contextId, message);
    QString fullMessage = logLine;

    if (!metadata.isEmpty()) {
        fullMessage += "\n" + formatMetadata(metadata);
    }

    if (m_consoleEnabled) {
        writeToConsole(fullMessage);
    }

    if (m_fileEnabled) {
        writeToFile(fullMessage);
    }

    emit logEmitted(level, fullMessage);
}

void Logger::debug(const QString& source, const QString& contextId,
                   const QString& message, const QJsonObject& meta)
{
    log(DEBUG, source, contextId, message, meta);
}

void Logger::info(const QString& source, const QString& contextId,
                  const QString& message, const QJsonObject& meta)
{
    log(INFO, source, contextId, message, meta);
}

void Logger::warn(const QString& source, const QString& contextId,
                  const QString& message, const QJsonObject& meta)
{
    log(WARN, source, contextId, message, meta);
}

void Logger::error(const QString& source, const QString& contextId,
                   const QString& message, const QJsonObject& meta)
{
    log(ERROR, source, contextId, message, meta);
}

void Logger::fatal(const QString& source, const QString& contextId,
                   const QString& message, const QJsonObject& meta)
{
    log(FATAL, source, contextId, message, meta);
}

void Logger::logRequest(const QString& source, const QString& requestId,
                        const QString& operation, const QString& entity,
                        const QJsonObject& params)
{
    QJsonObject metadata;
    metadata["requestId"] = requestId;
    metadata["operation"] = operation;
    metadata["entity"] = entity;
    metadata["params"] = params;

    log(INFO, source, requestId, "[Request]", metadata);
}

void Logger::logResponse(const QString& source, const QString& requestId,
                         const QString& operation, const QString& entity,
                         qint64 durationMs, bool success,
                         const QJsonObject& result)
{
    QJsonObject metadata;
    metadata["requestId"] = requestId;
    metadata["operation"] = operation;
    metadata["entity"] = entity;
    metadata["durationMs"] = durationMs;
    metadata["success"] = success;

    for (auto it = result.begin(); it != result.end(); ++it) {
        metadata[it.key()] = it.value();
    }

    QString message = QString("[Response]: status=%1").arg(success ? "success" : "failed");
    log(INFO, source, requestId, message, metadata);
}

void Logger::logError(const QString& source, const QString& requestId,
                      const QString& operation, const QString& entity,
                      const QString& errorMessage, const QString& errorCode,
                      qint64 durationMs, const QString& stack)
{
    QJsonObject metadata;
    metadata["requestId"] = requestId;
    metadata["operation"] = operation;
    metadata["entity"] = entity;
    metadata["errorMessage"] = errorMessage;
    metadata["errorCode"] = errorCode;
    metadata["durationMs"] = durationMs;

    if (!stack.isEmpty()) {
        metadata["stack"] = stack;
    }

    QString message = QString("[Error]: %1").arg(errorMessage);
    log(ERROR, source, requestId, message, metadata);
}

void Logger::logTransactionStart(const QString& transactionId,
                                 const QString& initiatedBy,
                                 const QString& requestId)
{
    QJsonObject metadata;
    metadata["transactionId"] = transactionId;
    metadata["initiatedBy"] = initiatedBy;
    metadata["requestId"] = requestId;

    log(INFO, "DatabaseManager::beginTransaction", transactionId,
        "Transaction started", metadata);
}

void Logger::logTransactionEnd(const QString& transactionId,
                               const QString& requestId,
                               qint64 durationMs,
                               bool committed,
                               const QStringList& operations)
{
    QJsonObject metadata;
    metadata["transactionId"] = transactionId;
    metadata["requestId"] = requestId;
    metadata["durationMs"] = durationMs;
    metadata["committed"] = committed;

    QJsonArray opsArray;
    for (const QString& op : operations) {
        opsArray.append(op);
    }
    metadata["operations"] = opsArray;

    QString message = committed ? "Transaction committed" : "Transaction rolled back";
    log(INFO, "DatabaseManager::endTransaction", transactionId, message, metadata);
}

void Logger::logQuery(const QString& source, const QString& requestId,
                      const QString& sql, const QVariantList& bindValues)
{
    QJsonObject metadata;
    metadata["requestId"] = requestId;
    metadata["sql"] = sql;

    QJsonArray valuesArray;
    for (const QVariant& value : bindValues) {
        valuesArray.append(QJsonValue::fromVariant(value));
    }
    metadata["bindValues"] = valuesArray;

    log(DEBUG, source, requestId, "Executing SQL", metadata);
}

QString Logger::formatLogLine(Level level, const QString& source,
                              const QString& contextId, const QString& message)
{
    return QString("[%1] [%2] [%3] [%4] > %5")
    .arg(getCurrentTimestamp())
        .arg(levelToString(level))
        .arg(source)
        .arg(contextId)
        .arg(message);
}

QString Logger::formatMetadata(const QJsonObject& metadata)
{
    QJsonDocument doc(metadata);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

QString Logger::getCurrentTimestamp()
{
    return QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ss.zzz") + "Z";
}

QString Logger::levelToString(Level level)
{
    switch (level) {
    case DEBUG: return "DEBUG";
    case INFO:  return "INFO";
    case WARN:  return "WARN";
    case ERROR: return "ERROR";
    case FATAL: return "FATAL";
    default:    return "UNKNOWN";
    }
}

void Logger::writeToConsole(const QString& message)
{
    qDebug().noquote() << message;
}

void Logger::writeToFile(const QString& message)
{
    if (m_fileStream) {
        *m_fileStream << message << "\n";
        m_fileStream->flush();
    }
}

void Logger::checkDailyRotation()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    if (m_lastRotationDate != today) {
        rotateLogFileIfNeeded();
        m_lastRotationDate = today;
    }
}

void Logger::rotateLogFileIfNeeded()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString expectedFileName = QString("nimo_%1.log").arg(today);
    QString currentLogPath = QDir(m_currentLogDir).filePath(expectedFileName);

    if (m_currentLogFile && m_currentLogFile->fileName() != currentLogPath) {
        closeCurrentLogFile();
        moveCurrentLogToArchive();
        openNewLogFile(currentLogPath);
        cleanupOldLogs();
    } else if (!m_currentLogFile) {
        openNewLogFile(currentLogPath);
    }
}

void Logger::closeCurrentLogFile()
{
    if (m_fileStream) {
        m_fileStream->flush();
        delete m_fileStream;
        m_fileStream = nullptr;
    }
    if (m_currentLogFile) {
        m_currentLogFile->close();
    }
}

void Logger::moveCurrentLogToArchive()
{
    if (!m_currentLogFile) return;

    QString sourceFile = m_currentLogFile->fileName();
    QString fileName = QFileInfo(sourceFile).fileName();
    QString destFile = QDir(m_oldLogsDir).filePath(fileName);

    delete m_currentLogFile;
    m_currentLogFile = nullptr;

    QFile::rename(sourceFile, destFile);
}

void Logger::openNewLogFile(const QString& filePath)
{
    m_currentLogFile = new QFile(filePath);

    if (!m_currentLogFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qCritical() << "Failed to open log file:" << filePath;
        delete m_currentLogFile;
        m_currentLogFile = nullptr;
        return;
    }

    m_fileStream = new QTextStream(m_currentLogFile);
    m_fileStream->setCodec("UTF-8");

    *m_fileStream << "=== Log started at "
                  << QDateTime::currentDateTime().toString(Qt::ISODate)
                  << " ===" << "\n";
    m_fileStream->flush();
}

void Logger::cleanupOldLogs()
{
    QDir oldLogsDir(m_oldLogsDir);
    QStringList filters;
    filters << "nimo_*.log";

    QFileInfoList files = oldLogsDir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);

    QDate cutoffDate = QDate::currentDate().addDays(-m_maxDaysToKeep);

    for (const QFileInfo& fileInfo : files) {
        QString fileName = fileInfo.fileName();
        QRegularExpression datePattern("nimo_(\\d{4}-\\d{2}-\\d{2})\\.log");
        QRegularExpressionMatch match = datePattern.match(fileName);

        if (match.hasMatch()) {
            QString dateStr = match.captured(1);
            QDate fileDate = QDate::fromString(dateStr, "yyyy-MM-dd");

            if (fileDate.isValid() && fileDate < cutoffDate) {
                QFile::remove(fileInfo.absoluteFilePath());

                qDebug() << "Deleted old log file:" << fileName
                         << "Age:" << fileDate.daysTo(QDate::currentDate()) << "days";
            }
        }
    }
}
