#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStandardPaths>

#include "logger/logger.h"
#include "ActivityTrackerService.h"
#include "ServiceManager.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("ActivityTracker");
    QCoreApplication::setApplicationVersion("1.0.0");

    // Setup command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Activity Tracker Service");
    parser.addHelpOption();
    parser.addVersionOption();

    // Service control options
    QCommandLineOption installOption("install", "Install the service");
    QCommandLineOption uninstallOption("uninstall", "Uninstall the service");
    QCommandLineOption startOption("start", "Start the service");
    QCommandLineOption stopOption("stop", "Stop the service");
    QCommandLineOption consoleOption("console", "Run as console application (for debugging)");
    QCommandLineOption logFileOption("logfile", "Specify log file path", "path");
    QCommandLineOption logLevelOption("loglevel", "Set log level (debug, info, warning, error)", "level", "info");

    parser.addOption(installOption);
    parser.addOption(uninstallOption);
    parser.addOption(startOption);
    parser.addOption(stopOption);
    parser.addOption(consoleOption);
    parser.addOption(logFileOption);
    parser.addOption(logLevelOption);

    parser.process(app);

    // Initialize logger
    if (parser.isSet(logFileOption)) {
        Logger::instance()->setLogFile(parser.value(logFileOption));
    } else {
        // Default log file location
        QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(logDir);
        Logger::instance()->setLogFile(logDir + "/activity_tracker.log");
    }

    // Set log level
    if (parser.isSet(logLevelOption)) {
        QString logLevel = parser.value(logLevelOption).toLower();
        if (logLevel == "debug") {
            Logger::instance()->setLogLevel(Logger::Debug);
        } else if (logLevel == "info") {
            Logger::instance()->setLogLevel(Logger::Info);
        } else if (logLevel == "warning") {
            Logger::instance()->setLogLevel(Logger::Warning);
        } else if (logLevel == "error") {
            Logger::instance()->setLogLevel(Logger::Error);
        }
    } else {
        Logger::instance()->setLogLevel(Logger::Info);
    }

    LOG_INFO("Activity Tracker Service starting...");

    // Service control actions
    ServiceManager serviceManager;

    if (parser.isSet(installOption)) {
        LOG_INFO("Installing service...");
        bool success = serviceManager.installService();
        return success ? 0 : 1;
    }

    if (parser.isSet(uninstallOption)) {
        LOG_INFO("Uninstalling service...");
        bool success = serviceManager.uninstallService();
        return success ? 0 : 1;
    }

    if (parser.isSet(startOption)) {
        LOG_INFO("Starting service...");
        bool success = serviceManager.startService();
        return success ? 0 : 1;
    }

    if (parser.isSet(stopOption)) {
        LOG_INFO("Stopping service...");
        bool success = serviceManager.stopService();
        return success ? 0 : 1;
    }

    // Create and initialize the service
    ActivityTrackerService service;

    if (!service.initialize()) {
        LOG_ERROR("Failed to initialize service");
        return 1;
    }

    if (parser.isSet(consoleOption)) {
        // Run as console application
        LOG_INFO("Running in console mode");
        if (!service.start()) {
            LOG_ERROR("Failed to start service");
            return 1;
        }

        // Install event handler to gracefully shutdown the service
        QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
            LOG_INFO("Application shutting down...");
            service.stop();
        });

        return app.exec();
    } else {
        // Run as a system service
        if (serviceManager.runService(service)) {
            LOG_INFO("Service running...");
            return app.exec();
        } else {
            LOG_ERROR("Failed to run service");
            return 1;
        }
    }
}

