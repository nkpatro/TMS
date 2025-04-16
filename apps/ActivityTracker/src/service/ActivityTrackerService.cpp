// ActivityTrackerService.cpp
#include "ActivityTrackerService.h"
#include "core/APIManager.h"
#include "logger/logger.h"
#include <QCoreApplication>
#include <QHostInfo>
#include <QDir>
#include <QStandardPaths>
#include <csignal>

// Signal handler for graceful shutdown
void signalHandler(int signal)
{
    LOG_INFO(QString("Received signal: %1").arg(signal));
    QCoreApplication::quit();
}

ActivityTrackerService::ActivityTrackerService(QObject *parent)
    : QObject(parent)
    , m_trackerClient(nullptr)
    , m_userManager(nullptr)
    , m_configManager(nullptr)
    , m_isRunning(false)
    , m_serverUrl("http://localhost:8080")
    , m_dataSendInterval(60000)    // 1 minute
    , m_idleTimeThreshold(300000)  // 5 minutes
    , m_trackKeyboardMouse(true)
    , m_trackApplications(true)
    , m_trackSystemMetrics(true)
    , m_multiUserMode(false)
    , m_machineId("")
{
    // Setup signal handlers for graceful shutdown
    setupSignalHandlers();

    // Set up heartbeat timer
    connect(&m_heartbeatTimer, &QTimer::timeout, [this]() {
        LOG_DEBUG("Service heartbeat");
    });
    m_heartbeatTimer.setInterval(300000); // 5 minutes
}

ActivityTrackerService::~ActivityTrackerService()
{
    if (m_isRunning) {
        stop();
    }
}

bool ActivityTrackerService::initialize()
{
    LOG_INFO("Initializing ActivityTrackerService");

    // Create config manager
    m_configManager = new ConfigManager(this);
    if (!m_configManager->initialize()) { // No API manager yet
        LOG_ERROR("Failed to initialize ConfigManager");
        return false;
    }

    // Load configuration
    if (!loadConfig()) {
        LOG_ERROR("Failed to load configuration");
        return false;
    }

    // Connect configuration change signals
    //connect(m_configManager, &ConfigManager::configChanged,
    //    this, &ActivityTrackerService::onConfigChanged);

    // Initialize user manager if multi-user mode is enabled
    if (m_multiUserMode) {
        m_userManager = new MultiUserManager(this);
        connect(m_userManager, &MultiUserManager::userSessionChanged,
                this, &ActivityTrackerService::onUserSessionChanged);

        if (!m_userManager->initialize()) {
            LOG_ERROR("Failed to initialize MultiUserManager");
            return false;
        } else
        {
            m_currentUser = m_userManager->currentUser();
            if (m_currentUser.isEmpty()) {
                QStringList activeUsers = m_userManager->activeUsers();
                if (!activeUsers.isEmpty()) {
                    m_currentUser = activeUsers.first();
                    LOG_INFO(QString("Current user not set, using first active user: %1").arg(m_currentUser));
                }
            }
        }
    }

    // Final check for empty username
    if (m_currentUser.isEmpty()) {
        m_currentUser = "unknown";
        LOG_WARNING("Still could not determine current user, using 'unknown'");
    } else {
        LOG_INFO(QString("Using username: %1").arg(m_currentUser));
    }

    // Create tracker client
    m_trackerClient = new ActivityTrackerClient(this);
    m_trackerClient->setConfigManager(m_configManager);

    // Connect client signals
    connect(m_trackerClient, &ActivityTrackerClient::statusChanged,
            this, &ActivityTrackerService::onStatusChanged);
    connect(m_trackerClient, &ActivityTrackerClient::errorOccurred,
            this, &ActivityTrackerService::onErrorOccurred);

    // Initialize tracker client with server status check first
    if (!m_trackerClient->initialize(m_serverUrl, m_currentUser, m_machineId)) {
        LOG_ERROR("Failed to initialize ActivityTrackerClient");
        return false;
    }

    if (m_multiUserMode && m_userManager && m_trackerClient) {
        m_trackerClient->setMultiUserManager(m_userManager);
    }

    // Apply configuration settings
    m_trackerClient->setDataSendInterval(m_dataSendInterval);

    // Set idle time threshold if tracking keyboard/mouse
    if (m_trackKeyboardMouse) {
        m_trackerClient->setIdleTimeThreshold(m_idleTimeThreshold);
    }

    LOG_INFO("ActivityTrackerService initialized successfully");
    return true;
}

bool ActivityTrackerService::start()
{
    if (m_isRunning) {
        LOG_WARNING("ActivityTrackerService is already running");
        return true;
    }

    LOG_INFO("Starting ActivityTrackerService");

    // Start user manager if multi-user mode is enabled
    if (m_multiUserMode && m_userManager) {
        if (!m_userManager->start()) {
            LOG_ERROR("Failed to start MultiUserManager");
            return false;
        }
    }

    // Start tracker client
    if (!m_trackerClient->start()) {
        LOG_ERROR("Failed to start ActivityTrackerClient");
        return false;
    }

    // Start heartbeat timer
    m_heartbeatTimer.start();
    m_isRunning = true;

    LOG_INFO(QString("ActivityTrackerService started successfully (User: %1)").arg(m_currentUser));
    return true;
}

bool ActivityTrackerService::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("ActivityTrackerService is not running");
        return true;
    }

    LOG_INFO("Stopping ActivityTrackerService");

    // Stop heartbeat timer
    m_heartbeatTimer.stop();

    // Stop tracker client
    if (m_trackerClient) {
        m_trackerClient->stop();
    }

    // Stop user manager if multi-user mode is enabled
    if (m_multiUserMode && m_userManager) {
        m_userManager->stop();
    }

    m_isRunning = false;

    LOG_INFO("ActivityTrackerService stopped successfully");
    return true;
}

bool ActivityTrackerService::reload()
{
    LOG_INFO("Reloading ActivityTrackerService");

    bool wasRunning = m_isRunning;

    // Stop service if it's running
    if (wasRunning) {
        stop();
    }

    // Reload configuration
    if (!loadConfig()) {
        LOG_ERROR("Failed to reload configuration");
        return false;
    }

    // Reload the tracker client
    if (m_trackerClient) {
        m_trackerClient->reload();
    }

    // Restart service if it was running
    if (wasRunning) {
        return start();
    }

    return true;
}

bool ActivityTrackerService::isRunning() const
{
    return m_isRunning;
}

void ActivityTrackerService::onConfigChanged()
{
    LOG_INFO("Configuration changed, reloading");
    reload();
}

void ActivityTrackerService::onUserSessionChanged(const QString& username, bool active)
{
    LOG_INFO(QString("User session changed: %1 (active: %2)").arg(username).arg(active));

    if (active && username != m_currentUser) {
        // A new user has become active, switch to them

        // First, if we're running, stop tracking for current user
        bool wasRunning = m_isRunning;
        if (wasRunning) {
            m_trackerClient->stop();
        }

        // Update current user
        m_currentUser = username;

        // Re-initialize with new user
        m_trackerClient->initialize(m_serverUrl, m_currentUser, m_machineId);

        // Set the auth token if available - AFTER initialization
        if (m_multiUserMode && m_userManager && m_userManager->hasUserAuthToken(username)) {
            // We have an auth token, pass it to the client's APIManager
            QString token = m_userManager->getUserAuthToken(username);
            if (!token.isEmpty()) {
                LOG_INFO(QString("Setting existing auth token for user: %1").arg(username));

                APIManager* apiManager = m_trackerClient->apiManager();
                if (apiManager) {
                    apiManager->setAuthToken(token);
                    LOG_INFO("Auth token set in APIManager");
                } else {
                    LOG_WARNING("Could not get APIManager to set auth token");
                }
            }
        }

        // Restart if service was running
        if (wasRunning) {
            m_trackerClient->start();
        }
    }
    else if (!active && username == m_currentUser) {
        // Current user has become inactive (locked screen, logged out, etc.)
        // We should pause tracking but keep authentication token

        if (m_isRunning) {
            LOG_INFO(QString("Current user '%1' inactive, pausing tracking").arg(username));

            // Before stopping, capture the auth token if it isn't already stored
            if (m_multiUserMode && m_userManager && !m_userManager->hasUserAuthToken(username)) {
                APIManager* apiManager = m_trackerClient->apiManager();
                if (apiManager) {
                    QString token = apiManager->getAuthToken();
                    if (!token.isEmpty()) {
                        m_userManager->setUserAuthToken(username, token);
                        LOG_INFO(QString("Captured auth token for user '%1' before pausing").arg(username));
                    }
                }
            }

            m_trackerClient->stop();
        }
    }
}

void ActivityTrackerService::onStatusChanged(const QString& status)
{
    LOG_INFO(QString("Client status changed: %1").arg(status));
}

void ActivityTrackerService::onErrorOccurred(const QString& errorMessage)
{
    LOG_ERROR(QString("Client error: %1").arg(errorMessage));
}

bool ActivityTrackerService::loadConfig()
{
    LOG_INFO("Loading configuration");

    if (!m_configManager) {
        LOG_ERROR("Config manager not initialized");
        return false;
    }

    // Load from configuration file
    if (!m_configManager->loadLocalConfig()) {
        LOG_WARNING("Failed to load configuration file, using defaults");
    }

    // Apply settings from configuration
    m_serverUrl = m_configManager->serverUrl();
    m_dataSendInterval = m_configManager->dataSendInterval();
    m_idleTimeThreshold = m_configManager->idleTimeThreshold();
    m_trackKeyboardMouse = m_configManager->trackKeyboardMouse();
    m_trackApplications = m_configManager->trackApplications();
    m_trackSystemMetrics = m_configManager->trackSystemMetrics();
    m_multiUserMode = m_configManager->multiUserMode();
    m_machineId = m_configManager->machineId();

    LOG_INFO(QString("Loaded configuration: Server: %1, Multi-user: %2")
                .arg(m_serverUrl)
                .arg(m_multiUserMode ? "Yes" : "No"));

    return true;
}

void ActivityTrackerService::setupSignalHandlers()
{
    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifndef _WIN32
    std::signal(SIGHUP, signalHandler);
#endif
}

bool ActivityTrackerService::authenticateCurrentUser()
{
    if (m_currentUser.isEmpty() || m_machineId.isEmpty()) {
        LOG_ERROR("Cannot authenticate: username or machineId not set");
        return false;
    }

    // If we have a MultiUserManager, use it for authentication to avoid redundant auth calls
    if (m_multiUserMode && m_userManager) {
        APIManager* apiManager = m_trackerClient->apiManager();
        if (!apiManager) {
            LOG_ERROR("Cannot authenticate: APIManager not available");
            return false;
        }

        return m_userManager->authenticateUser(m_currentUser, m_machineId, apiManager);
    }
    else {
        // Fall back to direct client authentication - this will always do the API call
        return m_trackerClient->authenticate(m_currentUser, m_machineId);
    }
}

