#include "ApiServer.h"
#include <QDebug>
#include <QUuid>
#include "dbservice/dbmanager.h"
#include "Controllers/AuthController.h"
#include "Controllers/MachineController.h"
#include "Controllers/SessionController.h"
#include "Controllers/ApplicationController.h"
#include "Controllers/SystemMetricsController.h"
#include "Controllers/AppUsageController.h"
#include "Controllers/ActivityEventController.h"
#include "Controllers/SessionEventController.h"
#include "Controllers/BatchController.h"
#include "Controllers/ServerStatusController.h"
#include "Services/ADVerificationService.h"
#include "Repositories/UserRepository.h"
#include "Repositories/MachineRepository.h"
#include "Repositories/SessionRepository.h"
#include "Repositories/ActivityEventRepository.h"
#include "Repositories/AfkPeriodRepository.h"
#include "Repositories/ApplicationRepository.h"
#include "Repositories/AppUsageRepository.h"
#include "Repositories/SystemMetricsRepository.h"
#include "Repositories/SessionEventRepository.h"
#include "Repositories/UserRoleDisciplineRepository.h"
#include "Controllers/UserRoleDisciplineController.h"
#include "Core/AuthFramework.h"
#include <QTimer>

ApiServer::ApiServer(QObject *parent)
: QObject(parent),
m_port(0),
m_hostAddress(QHostAddress::Any),
m_initialized(false),
m_userRepository(nullptr),
m_machineRepository(nullptr),
m_sessionRepository(nullptr),
m_activityEventRepository(nullptr),
m_afkPeriodRepository(nullptr),
m_applicationRepository(nullptr),
m_appUsageRepository(nullptr),
m_systemMetricsRepository(nullptr),
m_sessionEventRepository(nullptr)
{
    LOG_INFO("ApiServer created");
}

ApiServer::~ApiServer()
{
    LOG_INFO("ApiServer destructor called");

    // Stop the server if it's running
    stop();

    // Clean up repositories
    cleanupRepositories();
}

bool ApiServer::initialize(const DbConfig& dbConfig)
{
    LOG_INFO("Initializing ApiServer");

    if (m_initialized) {
        LOG_INFO("ApiServer already initialized");
        return true;
    }

    // Initialize DbManager with provided configuration
    if (!DbManager::instance().initialize(dbConfig)) {
        LOG_FATAL("Failed to initialize database manager");
        emit errorOccurred("Failed to initialize database connection");
        return false;
    }

    LOG_INFO(QString("Database connection initialized to %1@%2:%3/%4")
             .arg(dbConfig.username(), dbConfig.host())
             .arg(dbConfig.port())
             .arg(dbConfig.database()));

    // Set up controllers
    try {
        setupControllers();

        // Schedule periodic token cleanup
        QTimer *tokenCleanupTimer = new QTimer(this);
        connect(tokenCleanupTimer, &QTimer::timeout, []() {
            LOG_INFO("Running scheduled token cleanup");
            AuthFramework::instance().purgeExpiredTokens();
        });
        tokenCleanupTimer->start(30 * 60 * 1000); // Run every 30 minutes

        // Perform initial token cleanup
        AuthFramework::instance().purgeExpiredTokens();

        m_initialized = true;
        LOG_INFO("ApiServer initialized successfully");
        return true;
    }
    catch (const std::exception& ex) {
        LOG_FATAL(QString("Exception during controller setup: %1").arg(ex.what()));
        emit errorOccurred(QString("Failed to initialize server: %1").arg(ex.what()));
        return false;
    }
}

bool ApiServer::start(quint16 port, const QHostAddress& address)
{
    LOG_INFO(QString("Starting ApiServer on %1:%2").arg(address.toString()).arg(port));

    if (!m_initialized) {
        LOG_ERROR("Cannot start server: not initialized");
        emit errorOccurred("Server not initialized");
        return false;
    }

    if (isRunning()) {
        LOG_INFO(QString("Server already running on %1:%2").arg(m_hostAddress.toString()).arg(m_port));
        return true;
    }

    bool success = m_server.start(port, address);

    if (!success) {
        LOG_FATAL(QString("Failed to start server on %1:%2").arg(address.toString()).arg(port));
        emit errorOccurred(QString("Failed to start server on %1:%2").arg(address.toString()).arg(port));
        return false;
    }

    m_port = port;
    m_hostAddress = address;
    LOG_INFO(QString("ApiServer started successfully on %1:%2").arg(m_hostAddress.toString()).arg(m_port));
    emit serverStarted(m_port);
    return true;
}

bool ApiServer::stop()
{
    if (!isRunning()) {
        LOG_INFO("Server already stopped");
        return true;
    }

    LOG_INFO(QString("Stopping ApiServer on %1:%2").arg(m_hostAddress.toString()).arg(m_port));

    // Stop the server
    m_server.stop();

    // Mark as stopped
    m_port = 0;
    m_hostAddress = QHostAddress::Any;

    LOG_INFO("ApiServer stopped");
    emit serverStopped();
    return true;
}

bool ApiServer::isRunning() const
{
    return m_server.isRunning();
}

quint16 ApiServer::port() const
{
    return m_port;
}

QHostAddress ApiServer::hostAddress() const
{
    return m_hostAddress;
}

void ApiServer::setupControllers()
{
    LOG_INFO("Setting up controllers");

    try {
        // Create repositories
        LOG_DEBUG("Creating repositories");

        m_userRepository = new UserRepository(this);
        m_userRepository->initialize(&DbManager::instance().getService<UserModel>());

        m_machineRepository = new MachineRepository(this);
        m_machineRepository->initialize(&DbManager::instance().getService<MachineModel>());

        m_sessionRepository = new SessionRepository(this);
        m_sessionRepository->initialize(&DbManager::instance().getService<SessionModel>());

        m_activityEventRepository = new ActivityEventRepository(this);
        m_activityEventRepository->initialize(&DbManager::instance().getService<ActivityEventModel>());

        m_afkPeriodRepository = new AfkPeriodRepository(this);
        m_afkPeriodRepository->initialize(&DbManager::instance().getService<AfkPeriodModel>());

        m_applicationRepository = new ApplicationRepository(this);
        m_applicationRepository->initialize(&DbManager::instance().getService<ApplicationModel>());

        m_appUsageRepository = new AppUsageRepository(this);
        m_appUsageRepository->initialize(&DbManager::instance().getService<AppUsageModel>());

        m_systemMetricsRepository = new SystemMetricsRepository(this);
        m_systemMetricsRepository->initialize(&DbManager::instance().getService<SystemMetricsModel>());

        m_sessionEventRepository = new SessionEventRepository(this);
        m_sessionEventRepository->initialize(&DbManager::instance().getService<SessionEventModel>());

        m_userRoleDisciplineRepository = new UserRoleDisciplineRepository(this);
        m_userRoleDisciplineRepository->initialize(&DbManager::instance().getService<UserRoleDisciplineModel>());

        // Configure AuthFramework with repositories
        AuthFramework::instance().setUserRepository(m_userRepository);
        AuthFramework::instance().setAutoCreateUsers(true);
        AuthFramework::instance().setEmailDomain("redefine.co");

        // Set token expiry times
        AuthFramework::instance().setTokenExpiry(AuthFramework::UserToken, 24);      // 1 day
        AuthFramework::instance().setTokenExpiry(AuthFramework::ServiceToken, 168);  // 7 days
        AuthFramework::instance().setTokenExpiry(AuthFramework::ApiKey, 8760);       // 1 year
        AuthFramework::instance().setTokenExpiry(AuthFramework::RefreshToken, 720);  // 30 days

        LOG_DEBUG("Creating AD verification service");

        // Create and configure the AD verification service
        m_adVerificationService = std::make_shared<ADVerificationService>(this);
        m_adVerificationService->setADServerUrl("https://ad.redefine.co/api");

        LOG_DEBUG("Creating controllers");

        // Create AuthController with AD verification
        m_authController = std::make_shared<AuthController>(m_userRepository, m_adVerificationService.get(), this);

        // Configure AuthController
        m_authController->setAutoCreateUsers(true);
        m_authController->setEmailDomain("redefine.co");

        // Configure AuthFramework with AuthController
        AuthFramework::instance().setAuthController(m_authController.get());

        // Create other controllers
        m_machineController = std::make_shared<MachineController>(m_machineRepository, this);

        // Create SessionController with all required repositories
        m_sessionController = std::make_shared<SessionController>(
            m_sessionRepository,
            m_activityEventRepository,
            m_afkPeriodRepository,
            m_appUsageRepository,
            this);

        // Set additional repositories and controllers for SessionController
        m_sessionController->setAuthController(m_authController.get());
        m_sessionController->setSessionEventRepository(m_sessionEventRepository);
        m_sessionController->setMachineRepository(m_machineRepository);

        // Create ApplicationController
        m_applicationController = std::make_shared<ApplicationController>(
            m_applicationRepository,
            this);

        // Connect ApplicationController with AuthController
        m_applicationController->setAuthController(m_authController);

        // Create SystemMetricsController
        m_systemMetricsController = std::make_shared<SystemMetricsController>(
            m_systemMetricsRepository,
            this);

        // Set AuthController for SystemMetricsController
        m_systemMetricsController->setAuthController(m_authController.get());

        // Create AppUsageController
        m_appUsageController = std::make_shared<AppUsageController>(
            m_appUsageRepository,
            m_applicationRepository,
            this);

        // Set AuthController for AppUsageController
        m_appUsageController->setAuthController(m_authController.get());

        // Create ActivityEventController
        m_activityEventController = std::make_shared<ActivityEventController>(
            m_activityEventRepository,
            m_authController.get(),
            this);

        // Set SessionRepository in ActivityEventController for session validation
        m_activityEventController->setSessionRepository(m_sessionRepository);

        // Create SessionEventController
        m_sessionEventController = std::make_shared<SessionEventController>(
            m_sessionEventRepository,
            this);

        // Set AuthController for SessionEventController
        m_sessionEventController->setAuthController(m_authController.get());

        // Create UserRoleDisciplineController
        m_userRoleDisciplineController = std::make_shared<UserRoleDisciplineController>(
            m_userRoleDisciplineRepository,
            m_authController.get(),
            this);

        // Create BatchController
        m_batchController = std::make_shared<BatchController>(
            m_activityEventRepository,
            m_appUsageRepository,
            m_systemMetricsRepository,
            m_sessionEventRepository,
            m_sessionRepository,
            this);

        // Set AuthController for BatchController
        m_batchController->setAuthController(m_authController.get());

        // Create ServerStatusController
        m_serverStatusController = std::make_shared<ServerStatusController>(this);

        LOG_DEBUG("Registering controllers with server");

        // Register controllers with the server
        m_server.registerController(m_authController);
        m_server.registerController(m_machineController);
        m_server.registerController(m_sessionController);
        m_server.registerController(m_applicationController);
        m_server.registerController(m_systemMetricsController);
        m_server.registerController(m_appUsageController);
        m_server.registerController(m_activityEventController);
        m_server.registerController(m_sessionEventController);
        m_server.registerController(m_userRoleDisciplineController);
        m_server.registerController(m_batchController);
        m_server.registerController(m_serverStatusController);

        LOG_INFO("Controllers setup completed successfully");
    }
    catch (const std::exception& ex) {
        LOG_FATAL(QString("Exception during controller setup: %1").arg(ex.what()));
        cleanupRepositories(); // Clean up any partially initialized repositories
        throw; // Re-throw to allow proper handling by caller
    }
}

void ApiServer::cleanupRepositories()
{
    LOG_INFO("Cleaning up repositories");

    // Delete repositories in reverse order of creation to handle dependencies
    delete m_systemMetricsRepository;
    m_systemMetricsRepository = nullptr;

    delete m_appUsageRepository;
    m_appUsageRepository = nullptr;

    delete m_applicationRepository;
    m_applicationRepository = nullptr;

    delete m_afkPeriodRepository;
    m_afkPeriodRepository = nullptr;

    delete m_activityEventRepository;
    m_activityEventRepository = nullptr;

    delete m_sessionRepository;
    m_sessionRepository = nullptr;

    delete m_machineRepository;
    m_machineRepository = nullptr;

    delete m_userRepository;
    m_userRepository = nullptr;

    delete m_sessionEventRepository;
    m_sessionEventRepository = nullptr;

    delete m_userRoleDisciplineRepository;
    m_userRoleDisciplineRepository = nullptr;

    LOG_INFO("Repository cleanup completed");
}

