#ifndef APISERVER_H
#define APISERVER_H

#include <QObject>
#include <QString>
#include <QSqlError>
#include <QHostAddress>
#include <memory>
#include "httpserver/server.h"
#include "dbservice/dbconfig.h"
#include "logger/logger.h"

// Forward declarations for controllers
class AuthController;
class MachineController;
class SessionController;
class ApplicationController;
class SystemMetricsController;
class AppUsageController;
class ActivityEventController;
class SessionEventController;
class UserRoleDisciplineController;
class BatchController;
class ServerStatusController;

// Forward declarations for services
class ADVerificationService;

// Forward declarations for repositories
class UserRepository;
class MachineRepository;
class SessionRepository;
class ActivityEventRepository;
class AfkPeriodRepository;
class ApplicationRepository;
class AppUsageRepository;
class SystemMetricsRepository;
class SessionEventRepository;
class UserRoleDisciplineRepository;

class ApiServer : public QObject
{
    Q_OBJECT
public:
    explicit ApiServer(QObject *parent = nullptr);
    ~ApiServer();

    // Initialization
    bool initialize(const DbConfig& dbConfig);

    // Server management
    bool start(quint16 port = 8080, const QHostAddress& address = QHostAddress::Any);
    bool stop();
    bool isRunning() const;
    quint16 port() const;
    QHostAddress hostAddress() const;

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void requestReceived(const QString &endpoint, const QString &method);
    void errorOccurred(const QString &errorMessage);

private:
    Http::Server m_server;
    quint16 m_port;
    QHostAddress m_hostAddress;
    bool m_initialized;

    // Services
    std::shared_ptr<ADVerificationService> m_adVerificationService;

    // Controllers
    std::shared_ptr<AuthController> m_authController;
    std::shared_ptr<MachineController> m_machineController;
    std::shared_ptr<SessionController> m_sessionController;
    std::shared_ptr<ApplicationController> m_applicationController;
    std::shared_ptr<SystemMetricsController> m_systemMetricsController;
    std::shared_ptr<AppUsageController> m_appUsageController;
    std::shared_ptr<ActivityEventController> m_activityEventController;
    std::shared_ptr<SessionEventController> m_sessionEventController;
    std::shared_ptr<UserRoleDisciplineController> m_userRoleDisciplineController;
    std::shared_ptr<BatchController> m_batchController;
    std::shared_ptr<ServerStatusController> m_serverStatusController;

    // Repositories
    UserRepository* m_userRepository;
    MachineRepository* m_machineRepository;
    SessionRepository* m_sessionRepository;
    ActivityEventRepository* m_activityEventRepository;
    AfkPeriodRepository* m_afkPeriodRepository;
    ApplicationRepository* m_applicationRepository;
    AppUsageRepository* m_appUsageRepository;
    SystemMetricsRepository* m_systemMetricsRepository;
    SessionEventRepository* m_sessionEventRepository;
    UserRoleDisciplineRepository* m_userRoleDisciplineRepository;

    // Helper methods
    void setupControllers();
    void cleanupRepositories();
};

#endif // APISERVER_H