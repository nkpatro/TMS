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
#include "Repositories/TokenRepository.h"
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

        // Create all repositories first
        m_userRepository = new UserRepository(this);
        m_tokenRepository = new TokenRepository(this);
        m_machineRepository = new MachineRepository(this);
        m_sessionRepository = new SessionRepository(this);
        m_activityEventRepository = new ActivityEventRepository(this);
        m_afkPeriodRepository = new AfkPeriodRepository(this);
        m_applicationRepository = new ApplicationRepository(this);
        m_appUsageRepository = new AppUsageRepository(this);
        m_systemMetricsRepository = new SystemMetricsRepository(this);
        m_sessionEventRepository = new SessionEventRepository(this);
        m_userRoleDisciplineRepository = new UserRoleDisciplineRepository(this);

        // Initialize repositories
        LOG_DEBUG("Initializing repositories");
        m_userRepository->initialize(&DbManager::instance().getService<UserModel>());
        m_tokenRepository->initialize(&DbManager::instance().getService<TokenModel>());
        m_machineRepository->initialize(&DbManager::instance().getService<MachineModel>());
        m_sessionRepository->initialize(&DbManager::instance().getService<SessionModel>());
        m_activityEventRepository->initialize(&DbManager::instance().getService<ActivityEventModel>());
        m_afkPeriodRepository->initialize(&DbManager::instance().getService<AfkPeriodModel>());
        m_applicationRepository->initialize(&DbManager::instance().getService<ApplicationModel>());
        m_appUsageRepository->initialize(&DbManager::instance().getService<AppUsageModel>());
        m_systemMetricsRepository->initialize(&DbManager::instance().getService<SystemMetricsModel>());
        m_sessionEventRepository->initialize(&DbManager::instance().getService<SessionEventModel>());
        m_userRoleDisciplineRepository->initialize(&DbManager::instance().getService<UserRoleDisciplineModel>());

        // Link repositories that need references to each other
        // This is critical: Connect SessionEventRepository to SessionRepository BEFORE
        // creating controllers that might use these repositories
        if (m_sessionRepository && m_sessionEventRepository) {
            LOG_DEBUG("Linking SessionEventRepository to SessionRepository");
            m_sessionRepository->setSessionEventRepository(m_sessionEventRepository);

            // Verify the link is working properly
            bool hasEventRepository = m_sessionRepository->hasSessionEventRepository();
            LOG_INFO(QString("SessionRepository has event repository: %1").arg(hasEventRepository ? "YES" : "NO"));
        }

        // Configure AuthFramework
        AuthFramework::instance().setUserRepository(m_userRepository);
        AuthFramework::instance().setTokenRepository(m_tokenRepository);
        AuthFramework::instance().setAutoCreateUsers(true);
        AuthFramework::instance().setEmailDomain("redefine.co");

        // Initialize token storage (load from database into memory)
        AuthFramework::instance().initializeTokenStorage();

        // Set token expiry times
        AuthFramework::instance().setTokenExpiry(AuthFramework::UserToken, 24);
        AuthFramework::instance().setTokenExpiry(AuthFramework::ServiceToken, 168);
        AuthFramework::instance().setTokenExpiry(AuthFramework::ApiKey, 8760);
        AuthFramework::instance().setTokenExpiry(AuthFramework::RefreshToken, 720);

        // Create services
        LOG_DEBUG("Creating AD verification service");
        m_adVerificationService = std::make_shared<ADVerificationService>(this);
        m_adVerificationService->setADServerUrl("https://ad.redefine.co/api");

        // Create controllers
        LOG_DEBUG("Creating controllers");

        // Create AuthController first as it's needed by other controllers
        m_authController = std::make_shared<AuthController>(m_userRepository, m_adVerificationService.get(), this);
        m_authController->setTokenRepository(m_tokenRepository);
        m_authController->setAutoCreateUsers(true);
        m_authController->setEmailDomain("redefine.co");

        AuthFramework::instance().setAuthController(m_authController.get());

        // Create SessionEventController before SessionController
        // This ensures it's fully initialized before being used
        m_sessionEventController = std::make_shared<SessionEventController>(
            m_sessionEventRepository,
            this);
        m_sessionEventController->setAuthController(m_authController.get());

        // Do explicitly check if it's properly initialized
        if (!m_sessionEventController->initialize()) {
            LOG_ERROR("Failed to initialize SessionEventController");
            throw std::runtime_error("SessionEventController initialization failed");
        }
        LOG_INFO("SessionEventController initialized successfully");

        // Create remaining controllers
        m_machineController = std::make_shared<MachineController>(m_machineRepository, this);

        // Create SessionController with all required repositories
        m_sessionController = std::make_shared<SessionController>(
            m_sessionRepository,
            m_activityEventRepository,
            m_afkPeriodRepository,
            m_appUsageRepository,
            this);

        m_sessionController->setAuthController(m_authController.get());
        // This should be redundant since we already set it at repository level,
        // but do it anyway for clarity and to ensure proper linkage
        m_sessionController->setSessionEventRepository(m_sessionEventRepository);
        m_sessionController->setMachineRepository(m_machineRepository);

        // Initialize explicitly to verify everything is working
        if (!m_sessionController->initialize()) {
            LOG_ERROR("Failed to initialize SessionController");
            throw std::runtime_error("SessionController initialization failed");
        }
        LOG_INFO("SessionController initialized successfully");

        // Create remaining controllers...
        m_applicationController = std::make_shared<ApplicationController>(
            m_applicationRepository, this);
        m_applicationController->setAuthController(m_authController);

        m_systemMetricsController = std::make_shared<SystemMetricsController>(
            m_systemMetricsRepository, this);
        m_systemMetricsController->setAuthController(m_authController.get());

        m_appUsageController = std::make_shared<AppUsageController>(
            m_appUsageRepository,
            m_applicationRepository,
            this);
        m_appUsageController->setAuthController(m_authController.get());

        m_activityEventController = std::make_shared<ActivityEventController>(
            m_activityEventRepository,
            m_authController.get(),
            this);
        m_activityEventController->setSessionRepository(m_sessionRepository);

        m_userRoleDisciplineController = std::make_shared<UserRoleDisciplineController>(
            m_userRoleDisciplineRepository,
            m_authController.get(),
            this);

        m_batchController = std::make_shared<BatchController>(
            m_activityEventRepository,
            m_appUsageRepository,
            m_systemMetricsRepository,
            m_sessionEventRepository,
            m_sessionRepository,
            this);
        m_batchController->setAuthController(m_authController.get());

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

        // Create default admin user if needed
        QUuid adminUserId;
        if (m_authController) {
            adminUserId = m_authController->createDefaultAdminUser();
            LOG_INFO(QString("Default admin user setup %1")
                .arg(adminUserId.isNull() ? "failed" : "completed successfully"));
        }

        // Store the admin user ID in ModelFactory for use in default creation
        ModelFactory::setDefaultCreatedBy(adminUserId);

        LOG_INFO("Controllers setup completed successfully");
    }
    catch (const std::exception& ex) {
        LOG_FATAL(QString("Exception during controller setup: %1").arg(ex.what()));
        cleanupRepositories();
        throw;
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

    delete m_tokenRepository;
    m_tokenRepository = nullptr;

    LOG_INFO("Repository cleanup completed");
}

