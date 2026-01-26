#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QMutex>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();

    // Initialization
    bool initialize();
    void shutdown();

    // Connection management
    QSqlDatabase database() const;
    bool isConnected() const;
    QString lastError() const;

    // Transaction management
    bool beginTransaction();
    bool commit();
    bool rollback();
    bool isInTransaction() const;

    // Migration management
    bool runMigrations();
    int currentSchemaVersion();

    // Connection info
    QString connectionId() const { return m_connectionId; }
    QString databaseName() const { return m_databaseName; }

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
    void migrationCompleted(int version);

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool createConnection();
    bool testConnection();
    bool ensureSchemaExists();
    bool executeMigration(int version, const QString& sql);
    QString loadMigrationFile(int version);

    QSqlDatabase m_db;
    QString m_connectionId;
    QString m_host;
    int m_port;
    QString m_databaseName;
    QString m_userName;
    QString m_password;
    bool m_isConnected;
    bool m_inTransaction;
    QString m_lastError;
    QMutex m_mutex;
};

#endif
