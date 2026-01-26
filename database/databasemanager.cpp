#include "database/databasemanager.h"
#include "logging/logger.h"
#include "logging/requestcontext.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager()
    : QObject(nullptr)
    , m_host("localhost")
    , m_port(5433)
    , m_databaseName("nimo_local")
    , m_userName("postgres")
    , m_password("")
    , m_isConnected(false)
    , m_inTransaction(false)
{
    m_connectionId = QString("conn_%1").arg(QDateTime::currentMSecsSinceEpoch());
}

DatabaseManager::~DatabaseManager()
{
    shutdown();
}

bool DatabaseManager::initialize()
{
    QMutexLocker locker(&m_mutex);

    QString contextId = "db_init";
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    Logger::instance().info("DatabaseManager::initialize", contextId,
                            "Initializing database connection", {
                                {"host", m_host},
                                {"port", m_port},
                                {"database", m_databaseName},
                                {"user", m_userName}
                            });

    // Create connection
    if (!createConnection()) {
        qint64 duration = QDateTime::currentMSecsSinceEpoch() - startTime;
        Logger::instance().error("DatabaseManager::initialize", contextId,
                                 "Failed to create database connection", {
                                     {"errorMessage", m_lastError},
                                     {"durationMs", duration}
                                 });
        return false;
    }

    // Test connection
    if (!testConnection()) {
        qint64 duration = QDateTime::currentMSecsSinceEpoch() - startTime;
        Logger::instance().error("DatabaseManager::initialize", contextId,
                                 "Failed to test database connection", {
                                     {"errorMessage", m_lastError},
                                     {"durationMs", duration}
                                 });
        return false;
    }

    // Get server version
    QString serverVersion = "unknown";
    QSqlQuery query(m_db);
    if (query.exec("SELECT version()") && query.next()) {
        serverVersion = query.value(0).toString();
    }

    qint64 duration = QDateTime::currentMSecsSinceEpoch() - startTime;

    Logger::instance().info("DatabaseManager::initialize", contextId,
                            "Database connection established", {
                                {"durationMs", duration},
                                {"connectionId", m_connectionId},
                                {"serverVersion", serverVersion},
                                {"databaseName", m_databaseName}
                            });

    m_isConnected = true;
    emit connected();

    // Run migrations
    if (!runMigrations()) {
        Logger::instance().warn("DatabaseManager::initialize", contextId,
                                "Migrations failed or incomplete", {});
        // Don't fail initialization if migrations fail - might be already up to date
    }

    return true;
}

void DatabaseManager::shutdown()
{
    QMutexLocker locker(&m_mutex);

    if (m_isConnected) {
        Logger::instance().info("DatabaseManager::shutdown", "db_shutdown",
                                "Closing database connection", {
                                    {"connectionId", m_connectionId}
                                });

        if (m_inTransaction) {
            rollback();
        }

        m_db.close();
        m_isConnected = false;
        emit disconnected();

        Logger::instance().info("DatabaseManager::shutdown", "db_shutdown",
                                "Database connection closed", {});
    }
}

bool DatabaseManager::createConnection()
{
    // Remove old connection if exists
    if (QSqlDatabase::contains("nimo_main")) {
        QSqlDatabase::removeDatabase("nimo_main");
    }

    // Create new connection
    m_db = QSqlDatabase::addDatabase("QPSQL", "nimo_main");
    m_db.setHostName(m_host);
    m_db.setPort(m_port);
    m_db.setDatabaseName(m_databaseName);
    m_db.setUserName(m_userName);

    if (!m_password.isEmpty()) {
        m_db.setPassword(m_password);
    }

    // Set connection options
    m_db.setConnectOptions("connect_timeout=10");

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::testConnection()
{
    QSqlQuery query(m_db);
    if (!query.exec("SELECT 1")) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

QSqlDatabase DatabaseManager::database() const
{
    return m_db;
}

bool DatabaseManager::isConnected() const
{
    return m_isConnected;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

bool DatabaseManager::beginTransaction()
{
    QMutexLocker locker(&m_mutex);

    if (!m_isConnected) {
        m_lastError = "Database not connected";
        return false;
    }

    if (m_inTransaction) {
        m_lastError = "Transaction already in progress";
        return false;
    }

    QString txnId = "txn_" + QString::number(QDateTime::currentMSecsSinceEpoch(), 16);

    if (!m_db.transaction()) {
        m_lastError = m_db.lastError().text();
        Logger::instance().error("DatabaseManager::beginTransaction", txnId,
                                 "Failed to start transaction", {
                                     {"errorMessage", m_lastError}
                                 });
        return false;
    }

    m_inTransaction = true;

    Logger::instance().info("DatabaseManager::beginTransaction", txnId,
                            "Transaction started", {
                                {"transactionId", txnId},
                                {"connectionId", m_connectionId}
                            });

    return true;
}

bool DatabaseManager::commit()
{
    QMutexLocker locker(&m_mutex);

    if (!m_inTransaction) {
        m_lastError = "No active transaction";
        return false;
    }

    QString txnId = "txn_commit";
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    if (!m_db.commit()) {
        m_lastError = m_db.lastError().text();
        m_inTransaction = false;

        qint64 duration = QDateTime::currentMSecsSinceEpoch() - startTime;
        Logger::instance().error("DatabaseManager::commit", txnId,
                                 "Transaction commit failed", {
                                     {"errorMessage", m_lastError},
                                     {"durationMs", duration}
                                 });
        return false;
    }

    m_inTransaction = false;

    qint64 duration = QDateTime::currentMSecsSinceEpoch() - startTime;
    Logger::instance().info("DatabaseManager::commit", txnId,
                            "Transaction committed", {
                                {"durationMs", duration}
                            });

    return true;
}

bool DatabaseManager::rollback()
{
    QMutexLocker locker(&m_mutex);

    if (!m_inTransaction) {
        m_lastError = "No active transaction";
        return false;
    }

    QString txnId = "txn_rollback";

    if (!m_db.rollback()) {
        m_lastError = m_db.lastError().text();
        m_inTransaction = false;

        Logger::instance().error("DatabaseManager::rollback", txnId,
                                 "Transaction rollback failed", {
                                     {"errorMessage", m_lastError}
                                 });
        return false;
    }

    m_inTransaction = false;

    Logger::instance().info("DatabaseManager::rollback", txnId,
                            "Transaction rolled back", {});

    return true;
}

bool DatabaseManager::isInTransaction() const
{
    return m_inTransaction;
}

bool DatabaseManager::runMigrations()
{
    QString contextId = "db_migrations";

    Logger::instance().info("DatabaseManager::runMigrations", contextId,
                            "Starting database migrations", {});

    // Check if schema_migrations table exists
    QSqlQuery query(m_db);
    query.exec("SELECT EXISTS (SELECT 1 FROM information_schema.tables "
               "WHERE table_schema = 'public' AND table_name = 'schema_migrations')");

    bool tableExists = false;
    if (query.next()) {
        tableExists = query.value(0).toBool();
    }

    if (!tableExists) {
        Logger::instance().info("DatabaseManager::runMigrations", contextId,
                                "Schema migrations table does not exist - assuming fresh database", {});
        // Schema needs to be created - user should run the SQL script manually
        return true;
    }

    // Get current schema version
    int currentVersion = currentSchemaVersion();

    Logger::instance().info("DatabaseManager::runMigrations", contextId,
                            "Current schema version", {
                                {"version", currentVersion}
                            });

    // For now, we assume migrations are handled externally via SQL scripts
    // Future enhancement: implement incremental migrations

    return true;
}

int DatabaseManager::currentSchemaVersion()
{
    QSqlQuery query(m_db);
    query.exec("SELECT COALESCE(MAX(version), 0) FROM schema_migrations");

    if (query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

bool DatabaseManager::ensureSchemaExists()
{
    QSqlQuery query(m_db);

    // Check if goals table exists as a proxy for schema existence
    query.exec("SELECT EXISTS (SELECT 1 FROM information_schema.tables "
               "WHERE table_schema = 'public' AND table_name = 'goals')");

    if (query.next()) {
        return query.value(0).toBool();
    }

    return false;
}

bool DatabaseManager::executeMigration(int version, const QString& sql)
{
    QString contextId = QString("migration_%1").arg(version);
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    Logger::instance().info("DatabaseManager::executeMigration", contextId,
                            "Executing migration", {
                                {"version", version}
                            });

    if (!beginTransaction()) {
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        rollback();

        qint64 duration = QDateTime::currentMSecsSinceEpoch() - startTime;
        Logger::instance().error("DatabaseManager::executeMigration", contextId,
                                 "Migration failed", {
                                     {"version", version},
                                     {"errorMessage", m_lastError},
                                     {"durationMs", duration}
                                 });
        return false;
    }

    // Record migration
    query.prepare("INSERT INTO schema_migrations (version, name, applied_at) "
                  "VALUES (:version, :name, CURRENT_TIMESTAMP)");
    query.bindValue(":version", version);
    query.bindValue(":name", QString("migration_%1").arg(version));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        rollback();
        return false;
    }

    if (!commit()) {
        return false;
    }

    qint64 duration = QDateTime::currentMSecsSinceEpoch() - startTime;
    Logger::instance().info("DatabaseManager::executeMigration", contextId,
                            "Migration completed", {
                                {"version", version},
                                {"durationMs", duration}
                            });

    emit migrationCompleted(version);
    return true;
}

QString DatabaseManager::loadMigrationFile(int version)
{
    QString fileName = QString("migration_%1.sql").arg(version, 3, 10, QChar('0'));
    QString filePath = QCoreApplication::applicationDirPath() + "/migrations/" + fileName;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QString("Could not open migration file: %1").arg(filePath);
        return QString();
    }

    return QString::fromUtf8(file.readAll());
}
