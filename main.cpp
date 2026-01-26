#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "logging/logger.h"
#include "database/databasemanager.h"
#include "repositories/goalrepository.h"
#include "services/goalservice.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Set application info
    QCoreApplication::setOrganizationName("Nimo");
    QCoreApplication::setOrganizationDomain("nimo.app");
    QCoreApplication::setApplicationName("Nimo Habit Tracker");

    // ========================================================================
    // 1. Initialize Logger
    // ========================================================================
    Logger::instance().setLogLevel(Logger::INFO);
    Logger::instance().setConsoleEnabled(true);
    Logger::instance().setFileEnabled(true);
    Logger::instance().setMaxDaysToKeep(30);

    Logger::instance().info("main", "app_start", "Application starting", {
                                                                             {"version", "1.0.0"},
                                                                             {"platform", "Windows"}
                                                                         });

    // ========================================================================
    // 2. Initialize Database
    // ========================================================================
    if (!DatabaseManager::instance().initialize()) {
        Logger::instance().fatal("main", "app_start",
                                 "Failed to initialize database", {
                                     {"error", DatabaseManager::instance().lastError()}
                                 });
        return -1;
    }

    // ========================================================================
    // 3. Create Repositories
    // ========================================================================
    QSqlDatabase db = DatabaseManager::instance().database();
    GoalRepository* goalRepo = new GoalRepository(db);

    Logger::instance().info("main", "app_start", "Repositories initialized", {});

    // ========================================================================
    // 4. Create Services
    // ========================================================================
    GoalService* goalService = new GoalService(goalRepo);

    Logger::instance().info("main", "app_start", "Services initialized", {});

    // ========================================================================
    // 5. Setup QML Engine
    // ========================================================================
    QQmlApplicationEngine engine;

    // Expose services to QML
    QQmlContext* rootContext = engine.rootContext();
    rootContext->setContextProperty("goalService", goalService);
    rootContext->setContextProperty("logger", &Logger::instance());

    // Load main QML file
    const QUrl url(u"qrc:/Nimo/Main.qml"_qs);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            Logger::instance().fatal("main", "qml_load", "Failed to load QML", {});
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    Logger::instance().info("main", "app_start", "Application started successfully", {});

    // ========================================================================
    // 6. Run Application
    // ========================================================================
    int result = app.exec();

    // ========================================================================
    // 7. Cleanup
    // ========================================================================
    Logger::instance().info("main", "app_shutdown", "Application shutting down", {});

    delete goalService;
    delete goalRepo;

    DatabaseManager::instance().shutdown();

    Logger::instance().info("main", "app_shutdown", "Application shutdown complete", {});

    return result;
}
