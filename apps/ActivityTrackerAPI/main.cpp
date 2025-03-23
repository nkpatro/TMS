#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDir>
#include <QNetworkInterface>
#include <QTimer>
#include "Server/ApiServer.h"
#include "dbservice/dbconfig.h"
#include "logger/logger.h"
#include "Core/AuthFramework.h"

// Helper function to get the host's IP addresses
QStringList getHostAddresses() {
    QStringList addresses;
    QList<QHostAddress> hostAddresses = QNetworkInterface::allAddresses();

    for (const QHostAddress &address : hostAddresses) {
        // Filter out loopback and IPv6 addresses for clarity
        if (!address.isLoopback() && address.protocol() == QAbstractSocket::IPv4Protocol) {
            addresses.append(address.toString());
        }
    }

    // Always include localhost
    if (!addresses.contains("127.0.0.1")) {
        addresses.prepend("127.0.0.1");
    }

    return addresses;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("ActivityTrackerAPI");
    QCoreApplication::setApplicationVersion("1.0.0");

    // Initialize logger
    Logger::instance()->setLogLevel(Logger::Debug);
    Logger::instance()->enableConsoleOutput(true);

    QString logFilePath = "logs/activity_tracker_api.log";
    QDir().mkpath("logs"); // Ensure log directory exists
    Logger::instance()->setLogFile(logFilePath);

    LOG_INFO("Starting Activity Tracker API");
    LOG_INFO(QString("Application version: %1").arg(QCoreApplication::applicationVersion()));

    // Set up command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Activity Tracker REST API Server");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add port option
    QCommandLineOption portOption(QStringList() << "p" << "port",
                                 QCoreApplication::translate("main", "Port to listen on"),
                                 QCoreApplication::translate("main", "port"),
                                 "8080");
    parser.addOption(portOption);

    // Add config file option
    QCommandLineOption configOption(QStringList() << "c" << "config",
                                  QCoreApplication::translate("main", "Path to config file"),
                                  QCoreApplication::translate("main", "config"),
                                  "config/database.ini");
    parser.addOption(configOption);

    // Add log level option
    QCommandLineOption logLevelOption(QStringList() << "l" << "log-level",
                                    QCoreApplication::translate("main", "Log level (debug, info, warning, error, fatal)"),
                                    QCoreApplication::translate("main", "level"),
                                    "info");
    parser.addOption(logLevelOption);

    // Add host option (to specify which interface to bind to)
    QCommandLineOption hostOption(QStringList() << "h" << "host",
                                 QCoreApplication::translate("main", "Host interface to bind to (IP or 'all')"),
                                 QCoreApplication::translate("main", "host"),
                                 "all");
    parser.addOption(hostOption);

    // Add token cleanup interval option
    QCommandLineOption tokenCleanupOption(QStringList() << "t" << "token-cleanup",
                                        QCoreApplication::translate("main", "Token cleanup interval in minutes (default: 30)"),
                                        QCoreApplication::translate("main", "minutes"),
                                        "30");
    parser.addOption(tokenCleanupOption);

    // If no arguments were passed, print the syntax
    if (argc <= 1) {
        parser.showHelp();
        return 0;
    }

    // Process the command line arguments
    parser.process(app);

    // Get the port
    int port = parser.value(portOption).toInt();

    // Get the host
    QString host = parser.value(hostOption);

    // Get token cleanup interval
    int tokenCleanupMinutes = parser.value(tokenCleanupOption).toInt();
    if (tokenCleanupMinutes < 1) {
        tokenCleanupMinutes = 30; // Fallback to default if invalid
    }

    // Set log level
    QString logLevel = parser.value(logLevelOption).toLower();
    if (logLevel == "debug") {
        Logger::instance()->setLogLevel(Logger::Debug);
    } else if (logLevel == "info") {
        Logger::instance()->setLogLevel(Logger::Info);
    } else if (logLevel == "warning") {
        Logger::instance()->setLogLevel(Logger::Warning);
    } else if (logLevel == "error") {
        Logger::instance()->setLogLevel(Logger::Error);
    } else if (logLevel == "fatal") {
        Logger::instance()->setLogLevel(Logger::Fatal);
    }

    LOG_INFO(QString("Log level set to: %1").arg(logLevel));

    // Get database configuration
    QString configPath = parser.value(configOption);
    DbConfig dbConfig;

    if (QFile::exists(configPath)) {
        LOG_INFO(QString("Loading database configuration from: %1").arg(configPath));
        dbConfig = DbConfig::fromFile(configPath);
    } else {
        LOG_WARNING(QString("Config file not found: %1, using environment variables").arg(configPath));
        dbConfig = DbConfig::fromEnvironment();
    }

    LOG_INFO(QString("Database configuration: %1@%2:%3/%4")
             .arg(dbConfig.username(), dbConfig.host())
             .arg(dbConfig.port())
             .arg(dbConfig.database()));

    // Create and initialize the API server
    ApiServer server;

    // Connect signals and slots for notifications
    QObject::connect(&server, &ApiServer::serverStarted, [port, host](quint16 actualPort) {
        LOG_INFO(QString("Server started on port %1").arg(actualPort));

        // Get all available IP addresses
        QStringList addresses = getHostAddresses();

        LOG_INFO("Server is accessible at:");
        for (const QString &address : addresses) {
            LOG_INFO(QString("  http://%1:%2/").arg(address).arg(port));
        }

        if (host != "all") {
            LOG_INFO(QString("Server is bound to the interface: %1").arg(host));
        } else {
            LOG_INFO("Server is bound to all available interfaces");
        }

        LOG_INFO("Press Ctrl+C to quit");
    });

    QObject::connect(&server, &ApiServer::serverStopped, []() {
        LOG_INFO("Server stopped");
    });

    QObject::connect(&server, &ApiServer::errorOccurred, [](const QString &error) {
        LOG_ERROR(QString("Server error: %1").arg(error));
    });

    // Initialize and start the server
    bool initialized = server.initialize(dbConfig);
    if (!initialized) {
        LOG_FATAL("Failed to initialize API server");
        return 1;
    }

    // Perform initial token cleanup
    AuthFramework::instance().purgeExpiredTokens();

    // Set up separate token cleanup timer (in addition to the one in ApiServer)
    // This ensures cleanup happens even if ApiServer timer fails
    QTimer tokenCleanupTimer;
    QObject::connect(&tokenCleanupTimer, &QTimer::timeout, []() {
        LOG_INFO("Global token cleanup triggered");
        AuthFramework::instance().purgeExpiredTokens();
    });

    tokenCleanupTimer.start(tokenCleanupMinutes * 60 * 1000);

    // Determine the host to bind to
    QHostAddress bindAddress;
    if (host.toLower() == "all") {
        bindAddress = QHostAddress::Any;
    } else {
        bindAddress = QHostAddress(host);
        if (bindAddress.isNull()) {
            LOG_WARNING(QString("Invalid host address: %1, using Any").arg(host));
            bindAddress = QHostAddress::Any;
        }
    }

    bool started = server.start(port, bindAddress);
    if (!started) {
        LOG_FATAL(QString("Failed to start API server on %1:%2").arg(host).arg(port));
        return 1;
    }

    // Handle application shutdown
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        LOG_INFO("Application shutting down");
        server.stop();

        // Perform final token cleanup
        LOG_INFO("Performing final token cleanup");
        AuthFramework::instance().purgeExpiredTokens();
    });

    return app.exec();
}

