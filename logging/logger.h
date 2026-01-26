#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QJsonObject>
#include <QDateTime>
#include <QDate>

class Logger : public QObject
{
    Q_OBJECT

public:
    enum Level {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3,
        FATAL = 4
    };
    Q_ENUM(Level)

    // Singleton access
    static Logger& instance();

    // Configuration
    void setLogLevel(Level minLevel);
    void setConsoleEnabled(bool enabled);
    void setFileEnabled(bool enabled);
    void setLogDirectory(const QString& dir);
    void setMaxDaysToKeep(int days);

    // Main logging interface
    void log(Level level,
             const QString& source,
             const QString& contextId,
             const QString& message,
             const QJsonObject& metadata = QJsonObject());

    // Convenience methods
    void debug(const QString& source, const QString& contextId,
               const QString& message, const QJsonObject& meta = QJsonObject());
    void info(const QString& source, const QString& contextId,
              const QString& message, const QJsonObject& meta = QJsonObject());
    void warn(const QString& source, const QString& contextId,
              const QString& message, const QJsonObject& meta = QJsonObject());
    void error(const QString& source, const QString& contextId,
               const QString& message, const QJsonObject& meta = QJsonObject());
    void fatal(const QString& source, const QString& contextId,
               const QString& message, const QJsonObject& meta = QJsonObject());

    // Request/Response logging helpers
    void logRequest(const QString& source, const QString& requestId,
                    const QString& operation, const QString& entity,
                    const QJsonObject& params);

    void logResponse(const QString& source, const QString& requestId,
                     const QString& operation, const QString& entity,
                     qint64 durationMs, bool success,
                     const QJsonObject& result);

    void logError(const QString& source, const QString& requestId,
                  const QString& operation, const QString& entity,
                  const QString& errorMessage, const QString& errorCode,
                  qint64 durationMs, const QString& stack = QString());

    // Transaction logging
    void logTransactionStart(const QString& transactionId,
                             const QString& initiatedBy,
                             const QString& requestId);
    void logTransactionEnd(const QString& transactionId,
                           const QString& requestId,
                           qint64 durationMs,
                           bool committed,
                           const QStringList& operations);

    // SQL query logging
    void logQuery(const QString& source, const QString& requestId,
                  const QString& sql, const QVariantList& bindValues);

signals:
    void logEmitted(Level level, const QString& formattedMessage);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    QString formatLogLine(Level level, const QString& source,
                          const QString& contextId, const QString& message);
    QString formatMetadata(const QJsonObject& metadata);
    QString getCurrentTimestamp();
    QString levelToString(Level level);

    void writeToConsole(const QString& message);
    void writeToFile(const QString& message);
    void checkDailyRotation();
    void rotateLogFileIfNeeded();
    void closeCurrentLogFile();
    void moveCurrentLogToArchive();
    void openNewLogFile(const QString& filePath);
    void cleanupOldLogs();

    Level m_minLevel;
    bool m_consoleEnabled;
    bool m_fileEnabled;
    QString m_logDirectory;
    QString m_currentLogDir;
    QString m_oldLogsDir;
    int m_maxDaysToKeep;
    QString m_lastRotationDate;

    QFile* m_currentLogFile;
    QTextStream* m_fileStream;
    QMutex m_mutex;
};

#endif // LOGGER_H
