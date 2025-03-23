// ServiceManager.cpp
#include "ServiceManager.h"
#include "logger/logger.h"
#include <QProcess>
#include <QDir>
#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <winsvc.h>
#else
#include <QFile>
#endif

ServiceManager::ServiceManager(QObject *parent)
    : QObject(parent)
{
}

ServiceManager::~ServiceManager()
{
}

bool ServiceManager::installService()
{
    LOG_INFO("Installing service: " + serviceDisplayName());

#ifdef Q_OS_WIN
    // Windows service installation
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scManager) {
        LOG_ERROR(QString("Failed to open service manager: %1").arg(GetLastError()));
        return false;
    }

    SC_HANDLE service = CreateServiceW(
        scManager,
        serviceName().toStdWString().c_str(),
        serviceDisplayName().toStdWString().c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        serviceExecutable().toStdWString().c_str(),
        NULL, NULL, NULL, NULL, NULL);

    if (service) {
        // Set service description
        SERVICE_DESCRIPTION desc;
        desc.lpDescription = const_cast<LPWSTR>(serviceDescription().toStdWString().c_str());
        ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &desc);

        CloseServiceHandle(service);
        CloseServiceHandle(scManager);

        LOG_INFO("Service installed successfully");
        return true;
    } else {
        LOG_ERROR(QString("Failed to create service: %1").arg(GetLastError()));
        CloseServiceHandle(scManager);
        return false;
    }
#else
    // Linux/Mac service installation (using install scripts)
    QString installScript;

#ifdef Q_OS_MACOS
    installScript = QCoreApplication::applicationDirPath() + "/install-service.sh";
#else // Linux
    installScript = QCoreApplication::applicationDirPath() + "/install-service.sh";
#endif

    // Check if script exists
    if (!QFile::exists(installScript)) {
        LOG_ERROR("Installation script not found: " + installScript);
        return false;
    }

    // Make script executable
    QFile file(installScript);
    file.setPermissions(file.permissions() | QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);

    // Run installation script
    QProcess process;
    process.start(installScript);
    process.waitForFinished();

    if (process.exitCode() == 0) {
        LOG_INFO("Service installed successfully");
        return true;
    } else {
        QString error = QString::fromUtf8(process.readAllStandardError());
        LOG_ERROR("Failed to install service: " + error);
        return false;
    }
#endif
}

bool ServiceManager::uninstallService()
{
    LOG_INFO("Uninstalling service: " + serviceDisplayName());

#ifdef Q_OS_WIN
    // Windows service uninstallation
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        LOG_ERROR(QString("Failed to open service manager: %1").arg(GetLastError()));
        return false;
    }

    SC_HANDLE service = OpenServiceW(
        scManager,
        serviceName().toStdWString().c_str(),
        SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);

    if (!service) {
        LOG_ERROR(QString("Failed to open service: %1").arg(GetLastError()));
        CloseServiceHandle(scManager);
        return false;
    }

    // Stop the service if it's running
    SERVICE_STATUS status;
    ControlService(service, SERVICE_CONTROL_STOP, &status);

    // Wait for service to stop
    while (QueryServiceStatus(service, &status)) {
        if (status.dwCurrentState != SERVICE_STOP_PENDING) {
            break;
        }
        Sleep(500);
    }

    // Delete the service
    BOOL success = DeleteService(service);
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);

    if (success) {
        LOG_INFO("Service uninstalled successfully");
        return true;
    } else {
        LOG_ERROR(QString("Failed to delete service: %1").arg(GetLastError()));
        return false;
    }
#else
    // Linux/Mac service uninstallation (using uninstall scripts)
    QString uninstallScript;

#ifdef Q_OS_MACOS
    uninstallScript = QCoreApplication::applicationDirPath() + "/uninstall-service.sh";
#else // Linux
    uninstallScript = QCoreApplication::applicationDirPath() + "/uninstall-service.sh";
#endif

    // Check if script exists
    if (!QFile::exists(uninstallScript)) {
        LOG_ERROR("Uninstallation script not found: " + uninstallScript);
        return false;
    }

    // Make script executable
    QFile file(uninstallScript);
    file.setPermissions(file.permissions() | QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);

    // Run uninstallation script
    QProcess process;
    process.start(uninstallScript);
    process.waitForFinished();

    if (process.exitCode() == 0) {
        LOG_INFO("Service uninstalled successfully");
        return true;
    } else {
        QString error = QString::fromUtf8(process.readAllStandardError());
        LOG_ERROR("Failed to uninstall service: " + error);
        return false;
    }
#endif
}

bool ServiceManager::startService()
{
    LOG_INFO("Starting service: " + serviceDisplayName());

#ifdef Q_OS_WIN
    // Windows service startup
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        LOG_ERROR(QString("Failed to open service manager: %1").arg(GetLastError()));
        return false;
    }

    SC_HANDLE service = OpenServiceW(
        scManager,
        serviceName().toStdWString().c_str(),
        SERVICE_START);

    if (!service) {
        LOG_ERROR(QString("Failed to open service: %1").arg(GetLastError()));
        CloseServiceHandle(scManager);
        return false;
    }

    BOOL success = StartService(service, 0, NULL);
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);

    if (success) {
        LOG_INFO("Service started successfully");
        return true;
    } else {
        LOG_ERROR(QString("Failed to start service: %1").arg(GetLastError()));
        return false;
    }
#else
    // Linux/Mac service startup
    QProcess process;

#ifdef Q_OS_MACOS
    process.start("launchctl", QStringList() << "load" << "/Library/LaunchDaemons/com.activity_tracker.plist");
#else // Linux
    process.start("systemctl", QStringList() << "start" << "activity-tracker");
#endif

    process.waitForFinished();

    if (process.exitCode() == 0) {
        LOG_INFO("Service started successfully");
        return true;
    } else {
        QString error = QString::fromUtf8(process.readAllStandardError());
        LOG_ERROR("Failed to start service: " + error);
        return false;
    }
#endif
}

bool ServiceManager::stopService()
{
    LOG_INFO("Stopping service: " + serviceDisplayName());

#ifdef Q_OS_WIN
    // Windows service stop
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        LOG_ERROR(QString("Failed to open service manager: %1").arg(GetLastError()));
        return false;
    }

    SC_HANDLE service = OpenServiceW(
        scManager,
        serviceName().toStdWString().c_str(),
        SERVICE_STOP);

    if (!service) {
        LOG_ERROR(QString("Failed to open service: %1").arg(GetLastError()));
        CloseServiceHandle(scManager);
        return false;
    }

    SERVICE_STATUS status;
    BOOL success = ControlService(service, SERVICE_CONTROL_STOP, &status);
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);

    if (success) {
        LOG_INFO("Service stopped successfully");
        return true;
    } else {
        LOG_ERROR(QString("Failed to stop service: %1").arg(GetLastError()));
        return false;
    }
#else
    // Linux/Mac service stop
    QProcess process;

#ifdef Q_OS_MACOS
    process.start("launchctl", QStringList() << "unload" << "/Library/LaunchDaemons/com.activity_tracker.plist");
#else // Linux
    process.start("systemctl", QStringList() << "stop" << "activity-tracker");
#endif

    process.waitForFinished();

    if (process.exitCode() == 0) {
        LOG_INFO("Service stopped successfully");
        return true;
    } else {
        QString error = QString::fromUtf8(process.readAllStandardError());
        LOG_ERROR("Failed to stop service: " + error);
        return false;
    }
#endif
}

bool ServiceManager::runService(ActivityTrackerService &service)
{
    LOG_INFO("Running service: " + serviceDisplayName());

#ifdef Q_OS_WIN
    // Windows service implementation
    // For a more robust implementation, would need to implement a proper SERVICE_MAIN function
    // This simple implementation just starts the service directly
    return service.start();
#else
    // Linux/Mac service implementation (run in foreground)
    return service.start();
#endif
}

QString ServiceManager::serviceName() const
{
    return "ActivityTracker";
}

QString ServiceManager::serviceDisplayName() const
{
    return "Activity Tracker Service";
}

QString ServiceManager::serviceDescription() const
{
    return "Tracks user activity and application usage";
}

QString ServiceManager::serviceExecutable() const
{
    return QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
}