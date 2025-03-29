// ActivityTrackerClient.cpp
#include "ActivityTrackerClient.h"
#include "APIManager.h"
#include "SessionManager.h"
#include "SessionStateMachine.h"
#include "SyncManager.h"
#include "ActivityMonitorBatcher.h"
#include "../managers/ConfigManager.h"
#include "../managers/MonitorManager.h"
#include "logger/logger.h"
#include <QHostInfo>
#include <QSysInfo>
#include <QNetworkInterface>

#include "ApplicationCache.h"

ActivityTrackerClient::ActivityTrackerClient(QObject* parent)
    : QObject(parent)
    , m_apiManager(nullptr)
    , m_sessionManager(nullptr)
    , m_monitorManager(nullptr)
    , m_sessionStateMachine(nullptr)
    , m_syncManager(nullptr)
    , m_batcher(nullptr)
    , m_configManager(nullptr)
    , m_isRunning(false)
    , m_idleTimeThreshold(300000)  // 5 minutes
    , m_dataSendInterval(60000)    // 1 minute
{
    // Set up day check timer
    connect(&m_dayCheckTimer, &QTimer::timeout, this, &ActivityTrackerClient::checkDayChange);
    m_dayCheckTimer.setInterval(3600000); // Check once per hour

    m_currentSessionDay = QDate::currentDate();
}

ActivityTrackerClient::~ActivityTrackerClient()
{
    if (m_isRunning) {
        stop();
    }

    // Clean up allocated resources
    delete m_batcher;
    delete m_syncManager;
    delete m_sessionStateMachine;
    delete m_monitorManager;
    delete m_sessionManager;
    delete m_apiManager;
	//delete m_configManager; This not required as it is parented to the service
}

bool ActivityTrackerClient::initialize(const QString& serverUrl, const QString& username, const QString& machineId)
{
    LOG_INFO("Initializing ActivityTrackerClient");

    // Store credentials
    m_serverUrl = serverUrl;
    m_username = username;
    m_machineId = machineId;

    // 1. Initialize API Manager
    m_apiManager = new APIManager(this);
    if (!m_apiManager->initialize(m_serverUrl)) {
        LOG_ERROR("Failed to initialize API Manager");
        return false;
    }

    // 5. Apply config settings
    m_serverUrl = m_configManager->serverUrl();
    m_dataSendInterval = m_configManager->dataSendInterval();
    m_idleTimeThreshold = m_configManager->idleTimeThreshold();

    // Update machine ID if provided in config
    if (m_machineId.isEmpty()) {
        m_machineId = m_configManager->machineId();
    }
    else if (m_machineId != m_configManager->machineId()) {
        // Machine ID provided differs from config, update config
        m_configManager->setMachineId(m_machineId);
    }

    // 6. Initialize Session Manager
    m_sessionManager = new SessionManager(this);
    if (!m_sessionManager->initialize(m_apiManager, m_username, m_machineId)) {
        LOG_ERROR("Failed to initialize Session Manager");
        return false;
    }

    // 7. Check if machine is registered, register if needed
    bool machineRegistered = checkAndRegisterMachine();
    if (!machineRegistered) {
        LOG_WARNING("Machine registration failed, will operate in offline mode initially");
    }

    // 8. Attempt authentication with server
    QJsonObject authResponse;
    bool authenticated = m_apiManager->authenticate(m_username, m_machineId, authResponse);
    if (!authenticated) {
        LOG_WARNING("Authentication failed, will operate in offline mode initially");
    }

    // 9. Initialize Session State Machine
    m_sessionStateMachine = new SessionStateMachine(m_sessionManager, this);
    if (!m_sessionStateMachine->initialize()) {
        LOG_ERROR("Failed to initialize Session State Machine");
        return false;
    }

    // Connect session state signals
    connect(m_sessionStateMachine, &SessionStateMachine::stateChanged,
            [this](int newState, int oldState) {
              this->onSessionStateChanged(newState, oldState);
            });

    // 10. Initialize Sync Manager
    m_syncManager = new SyncManager(m_apiManager, m_sessionManager, this);
    if (!m_syncManager->initialize(m_dataSendInterval, 1000)) {
        LOG_ERROR("Failed to initialize Sync Manager");
        return false;
    }

    // Connect sync manager signals
    connect(m_syncManager, &SyncManager::connectionStateChanged,
        this, &ActivityTrackerClient::onConnectionStateChanged);
    connect(m_syncManager, &SyncManager::syncCompleted,
        this, &ActivityTrackerClient::onSyncCompleted);

    // 11. Initialize Activity Monitor Batcher
    m_batcher = new ActivityMonitorBatcher(this);
    int batchInterval = (m_dataSendInterval > 0) ? qMin(1000, m_dataSendInterval / 10) : 0;
    m_batcher->initialize(batchInterval);

    // Connect batcher signals
    connect(m_batcher, &ActivityMonitorBatcher::batchedKeyboardActivity,
        this, &ActivityTrackerClient::onBatchedKeyboardActivity);
    connect(m_batcher, &ActivityMonitorBatcher::batchedMouseActivity,
        this, &ActivityTrackerClient::onBatchedMouseActivity);
    connect(m_batcher, &ActivityMonitorBatcher::batchedAppActivity,
        this, &ActivityTrackerClient::onBatchedAppActivity);

    // 12. Initialize Monitor Manager
    m_monitorManager = new MonitorManager(this);
    if (!m_monitorManager->initialize(
        m_configManager->trackKeyboardMouse(),
        m_configManager->trackApplications(),
        m_configManager->trackSystemMetrics())) {
        LOG_ERROR("Failed to initialize Monitor Manager");
        return false;
    }

    // Connect raw monitor signals to batcher and other handlers
    m_monitorManager->connectMonitorSignals(m_batcher);

    // Connect processed monitor signals
    connect(m_monitorManager, &MonitorManager::systemMetricsUpdated,
        this, &ActivityTrackerClient::onSystemMetricsUpdated);
    connect(m_monitorManager, &MonitorManager::highCpuProcessDetected,
        this, &ActivityTrackerClient::onHighCpuProcessDetected);
    connect(m_monitorManager, &MonitorManager::sessionStateChanged,
            [this](int newState, const QString &username) {
              this->onSessionStateChanged(newState, username);
            });
    connect(m_monitorManager, &MonitorManager::afkStateChanged,
        this, &ActivityTrackerClient::onAfkStateChanged);

    // Set monitor configuration
    m_monitorManager->setIdleTimeThreshold(m_idleTimeThreshold);

    // 13. Initialize ApplicationCache if not already done by MonitorManager
    if (m_monitorManager && m_monitorManager->isTrackingApplications() && !m_monitorManager->appCache()) {
        ApplicationCache* appCache = new ApplicationCache(m_monitorManager);
        if (appCache->initialize(m_apiManager)) {
            LOG_INFO("Application cache initialized successfully");
            m_monitorManager->setAppCache(appCache);
        } else {
            LOG_WARNING("Failed to initialize application cache");
            delete appCache;
        }
    }

    m_currentSessionDay = QDate::currentDate();

    LOG_INFO("ActivityTrackerClient initialized successfully");
    return true;
}

bool ActivityTrackerClient::start()
{
    if (m_isRunning) {
        LOG_WARNING("ActivityTrackerClient is already running");
        return true;
    }

    LOG_INFO("Starting ActivityTrackerClient");

    // 1. Start the batcher
    m_batcher->start();

    // 2. Start the monitor manager
    if (!m_monitorManager->start()) {
        LOG_ERROR("Failed to start Monitor Manager");
        m_batcher->stop();
        return false;
    }

    // 3. Start the sync manager
    if (!m_syncManager->start()) {
        LOG_ERROR("Failed to start Sync Manager");
        m_monitorManager->stop();
        m_batcher->stop();
        return false;
    }

    // 4. Create or reopen session for today
    QUuid sessionId;
    QDateTime sessionStart;
    bool isNewSession;

    if (!m_syncManager->createOrReopenSession(QDate::currentDate(), sessionId, sessionStart, isNewSession)) {
        LOG_ERROR("Failed to create or reopen session");
        m_syncManager->stop();
        m_monitorManager->stop();
        m_batcher->stop();
        return false;
    }

    // 5. Start session in state machine
    m_sessionStateMachine->startSession(sessionId, sessionStart);

    // 6. Start day check timer
    m_dayCheckTimer.start();

    m_isRunning = true;
    emit statusChanged("Running");

    LOG_INFO(QString("ActivityTrackerClient started successfully (Session: %1)").arg(sessionId.toString()));
    return true;
}

bool ActivityTrackerClient::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("ActivityTrackerClient is not running");
        return true;
    }

    LOG_INFO("Stopping ActivityTrackerClient");

    // 1. Stop day check timer
    m_dayCheckTimer.stop();

    // 2. End current session
    m_sessionStateMachine->endSession();

    // 3. Stop monitor manager
    m_monitorManager->stop();

    // 4. Stop batcher
    m_batcher->stop();

    // 5. Final sync before stopping
    m_syncManager->forceSyncNow();

    // 6. Stop sync manager
    m_syncManager->stop();

    m_isRunning = false;
    emit statusChanged("Stopped");

    LOG_INFO("ActivityTrackerClient stopped successfully");
    return true;
}

bool ActivityTrackerClient::reload()
{
    LOG_INFO("Reloading ActivityTrackerClient");

    bool wasRunning = m_isRunning;

    // Stop if running
    if (wasRunning) {
        stop();
    }

    // Reload config
    m_configManager->loadLocalConfig();
    m_configManager->fetchServerConfig();

    // Reinitialize components with new settings
    m_dataSendInterval = m_configManager->dataSendInterval();
    m_idleTimeThreshold = m_configManager->idleTimeThreshold();

    // Update sync manager interval
    m_syncManager->initialize(m_dataSendInterval, 1000);

    // Update monitor configurations
    m_monitorManager->setIdleTimeThreshold(m_idleTimeThreshold);

    // Update batcher interval
    int batchInterval = (m_dataSendInterval > 0) ? qMin(1000, m_dataSendInterval / 10) : 0;
    m_batcher->initialize(batchInterval);

    // Restart if it was running
    if (wasRunning) {
        return start();
    }

    return true;
}

bool ActivityTrackerClient::isRunning() const
{
    return m_isRunning;
}

void ActivityTrackerClient::setDataSendInterval(int milliseconds)
{
    m_dataSendInterval = milliseconds;
    m_configManager->setDataSendInterval(milliseconds);
}

void ActivityTrackerClient::setIdleTimeThreshold(int milliseconds)
{
    m_idleTimeThreshold = milliseconds;
    m_configManager->setIdleTimeThreshold(milliseconds);

    if (m_monitorManager) {
        m_monitorManager->setIdleTimeThreshold(milliseconds);
    }
}

QString ActivityTrackerClient::serverUrl() const
{
    return m_serverUrl;
}

QString ActivityTrackerClient::username() const
{
    return m_username;
}

QString ActivityTrackerClient::machineId() const
{
    return m_machineId;
}

QUuid ActivityTrackerClient::sessionId() const
{
    return m_sessionStateMachine ? m_sessionStateMachine->currentSessionId() : QUuid();
}

QDateTime ActivityTrackerClient::lastSyncTime() const
{
    // This would need to be tracked in SyncManager and exposed
    return QDateTime::currentDateTime();
}

bool ActivityTrackerClient::isOfflineMode() const
{
    return m_syncManager ? m_syncManager->isOfflineMode() : false;
}

void ActivityTrackerClient::setConfigManager(ConfigManager* configManager) {
    // Store the reference and connect signals
    m_configManager = configManager;
    connect(m_configManager, &ConfigManager::configChanged,
        this, &ActivityTrackerClient::onConfigChanged);
}

void ActivityTrackerClient::onConfigChanged()
{
    LOG_INFO("Configuration changed, applying updates");

    // Lock to prevent recursion/reentry issues
    static bool isUpdating = false;
    if (isUpdating) {
        LOG_WARNING("Already processing config change, skipping recursive update");
        return;
    }

    isUpdating = true;

    // Update local settings from config
    m_serverUrl = m_configManager->serverUrl();
    m_dataSendInterval = m_configManager->dataSendInterval();
    m_idleTimeThreshold = m_configManager->idleTimeThreshold();

    // Apply settings to components (careful not to trigger more signals)
    if (m_monitorManager) {
        m_monitorManager->setIdleTimeThreshold(m_idleTimeThreshold);
    }

    if (m_syncManager) {
        // Avoid re-initializing if not necessary
        if (m_syncManager->syncInterval() != m_dataSendInterval) {
            m_syncManager->initialize(m_dataSendInterval, 1000);
        }
    }

    if (m_batcher) {
        int batchInterval = (m_dataSendInterval > 0) ? qMin(1000, m_dataSendInterval / 10) : 0;
        m_batcher->initialize(batchInterval);
    }

    isUpdating = false;
    LOG_INFO("Configuration updates applied successfully");
}

void ActivityTrackerClient::onMachineIdChanged(const QString& machineId)
{
    if (m_machineId != machineId) {
        LOG_INFO(QString("Machine ID changed from %1 to %2").arg(m_machineId, machineId));
        m_machineId = machineId;
    }
}

void ActivityTrackerClient::onBatchedKeyboardActivity(int keyPressCount)
{
    if (!m_isRunning) return;

    // Record keyboard activity
    QJsonObject eventData;
    eventData["type"] = "keyboard";
    eventData["count"] = keyPressCount;
    recordActivityEvent("keyboard", eventData);
}

void ActivityTrackerClient::onBatchedMouseActivity(const QList<QPoint>& positions, int clickCount)
{
    if (!m_isRunning) return;

    // Record mouse movements
    if (!positions.isEmpty()) {
        QJsonObject moveData;
        moveData["type"] = "move";
        moveData["count"] = positions.size();

        // Just record the last position for simplicity
        QPoint lastPos = positions.last();
        moveData["x"] = lastPos.x();
        moveData["y"] = lastPos.y();

        recordActivityEvent("mouse_move", moveData);
    }

    // Record mouse clicks
    if (clickCount > 0) {
        QJsonObject clickData;
        clickData["type"] = "click";
        clickData["count"] = clickCount;

        recordActivityEvent("mouse_click", clickData);
    }
}

void ActivityTrackerClient::onBatchedAppActivity(const QString &appName, const QString &windowTitle,
    const QString &executablePath, int focusChanges)
{
    if (!m_isRunning) return;

    // Only record if actually changed
    if (m_currentAppName != appName || m_currentWindowTitle != windowTitle || m_currentAppPath != executablePath) {
        // Get app ID - this is the key change
        QString appId = getAppId(appName, executablePath);

        // If there was a previous app, end its usage
        if (!m_currentAppName.isEmpty()) {
            QJsonObject endData;
            endData["app_name"] = m_currentAppName;
            endData["window_title"] = m_currentWindowTitle;
            endData["executable_path"] = m_currentAppPath;
            endData["action"] = "end";

            // Add app ID if we have it
            if (!m_currentAppId.isEmpty()) {
                endData["app_id"] = m_currentAppId;
            }

            QUuid sessionId = this->sessionId();
            if (!sessionId.isNull()) {
                endData["session_id"] = sessionId.toString().remove('{').remove('}');
                m_syncManager->queueData(SyncManager::DataType::AppUsage, sessionId, endData);
            }
        }

        // Start new app usage
        QJsonObject startData;
        startData["app_name"] = appName;
        startData["window_title"] = windowTitle;
        startData["executable_path"] = executablePath;
        startData["action"] = "start";

        // Add app ID if we have it
        if (!appId.isEmpty()) {
            startData["app_id"] = appId;
        }

        QUuid sessionId = this->sessionId();
        if (!sessionId.isNull()) {
            startData["session_id"] = sessionId.toString().remove('{').remove('}');
            m_syncManager->queueData(SyncManager::DataType::AppUsage, sessionId, startData);
        }

        // Update current app
        m_currentAppName = appName;
        m_currentWindowTitle = windowTitle;
        m_currentAppPath = executablePath;
        m_currentAppId = appId; // Store the app ID

        // Record activity event for app change
        QJsonObject eventData;
        eventData["app_name"] = appName;
        eventData["window_title"] = windowTitle;
        eventData["executable_path"] = executablePath;
        eventData["focus_changes"] = focusChanges;

        // Add app ID if available
        if (!appId.isEmpty()) {
            eventData["app_id"] = appId;
        }

        recordActivityEvent("app_changed", eventData);
    }
}

void ActivityTrackerClient::onSessionStateChanged(int newState, int oldState)
{
    if (!m_isRunning) return;

    LOG_INFO(QString("Session state changed from %1 to %2").arg(oldState).arg(newState));

    // Record event for state change
    QJsonObject eventData;
    eventData["old_state"] = oldState;
    eventData["new_state"] = newState;

    recordSessionEvent("state_change", eventData);

    // Emit state change
    emit sessionStateChanged(QString::number(newState));
}

void ActivityTrackerClient::onConnectionStateChanged(bool online)
{
    if (!m_isRunning) return;

    LOG_INFO(QString("Connection state changed: %1").arg(online ? "online" : "offline"));

    QJsonObject eventData;
    eventData["online"] = online;

    recordSessionEvent("connection_change", eventData);
    emit statusChanged(online ? "Online" : "Offline");
}

void ActivityTrackerClient::onSyncCompleted(bool success, int itemsProcessed)
{
    if (!m_isRunning) return;

    LOG_INFO(QString("Sync completed: %1 items processed, success: %2")
        .arg(itemsProcessed)
        .arg(success ? "true" : "false"));

    emit syncCompleted(success);
}

void ActivityTrackerClient::onSystemMetricsUpdated(float cpuUsage, float gpuUsage, float ramUsage)
{
    if (!m_isRunning) return;

    recordSystemMetrics(cpuUsage, gpuUsage, ramUsage);
}

void ActivityTrackerClient::onHighCpuProcessDetected(const QString& processName, float cpuUsage)
{
    if (!m_isRunning) return;

    // Record high CPU usage as an event
    QJsonObject eventData;
    eventData["process_name"] = processName;
    eventData["cpu_usage"] = cpuUsage;
    eventData["type"] = "high_cpu";

    recordActivityEvent("system_alert", eventData);
}

void ActivityTrackerClient::onSessionStateChanged(int newState, const QString& username)
{
    if (!m_isRunning) return;

    LOG_INFO(QString("Session monitor state changed: %1 for user %2")
        .arg(newState)
        .arg(username));

    QJsonObject eventData;
    eventData["username"] = username;

    // Handle different session states
    switch (newState) {
    case 1: // Login
        recordSessionEvent("login", eventData);
        break;

    case 2: // Logout
        recordSessionEvent("logout", eventData);
        break;

    case 3: // Lock
        recordSessionEvent("lock", eventData);
        m_sessionStateMachine->systemSuspending();
        break;

    case 4: // Unlock
        recordSessionEvent("unlock", eventData);
        m_sessionStateMachine->systemResuming();
        break;

    case 6: // RemoteConnect
        eventData["is_remote"] = true;
        recordSessionEvent("remote_connect", eventData);
        break;

    case 7: // RemoteDisconnect
        eventData["is_remote"] = true;
        recordSessionEvent("remote_disconnect", eventData);
        break;

    case 5: {// SwitchUser
        // Record the switch with previous user info
        QJsonObject switchEventData;
        switchEventData["previous_username"] = m_username;
        switchEventData["new_username"] = username;

        recordSessionEvent("switch_user", switchEventData);

        // Update current username
        m_username = username;
        break;
        }
    default:
        break;
    }
}

void ActivityTrackerClient::onAfkStateChanged(bool isAfk)
{
    if (!m_isRunning) return;

    LOG_INFO(QString("AFK state changed: %1").arg(isAfk ? "away" : "active"));

    // Update session state machine
    m_sessionStateMachine->userAfk(isAfk);

    // Record activity event
    QJsonObject eventData;
    eventData["is_afk"] = isAfk;

    if (isAfk) {
        eventData["reason"] = "idle_timeout";
        recordActivityEvent("afk_start", eventData);
    }
    else {
        eventData["reason"] = "user_activity";
        recordActivityEvent("afk_end", eventData);
    }
}

void ActivityTrackerClient::checkDayChange()
{
    if (!m_isRunning) return;

    QDate currentDate = QDate::currentDate();

    // Check if we've crossed to a new day
    if (currentDate != m_currentSessionDay) {
        LOG_INFO("Day change detected, handling session transition");
        handleDayChange();
    }
}

bool ActivityTrackerClient::handleDayChange()
{
    LOG_INFO("Handling day change procedure");

    // 1. End the current session
    m_sessionStateMachine->endSession();

    // 2. Create a new session for today
    QUuid sessionId;
    QDateTime sessionStart;
    bool isNewSession;

    if (!m_syncManager->createOrReopenSession(QDate::currentDate(), sessionId, sessionStart, isNewSession)) {
        LOG_ERROR("Failed to create new session for the new day");
        return false;
    }

    // 3. Start the new session
    m_sessionStateMachine->startSession(sessionId, sessionStart);

    // 4. Update current day
    m_currentSessionDay = QDate::currentDate();

    LOG_INFO("Day change handled successfully");
    return true;
}

bool ActivityTrackerClient::recordSessionEvent(const QString& eventType, const QJsonObject& eventData)
{
    QUuid sessionId = this->sessionId();
    if (sessionId.isNull()) {
        LOG_WARNING("Cannot record session event: no active session");
        return false;
    }

    QJsonObject data = eventData;
    data["event_type"] = eventType;

    return m_syncManager->queueData(SyncManager::DataType::SessionEvent, sessionId, data);
}

bool ActivityTrackerClient::recordActivityEvent(const QString& eventType, const QJsonObject& eventData)
{
    QUuid sessionId = this->sessionId();
    if (sessionId.isNull()) {
        LOG_WARNING("Cannot record activity event: no active session");
        return false;
    }

    QJsonObject data = eventData;
    data["event_type"] = eventType;

    return m_syncManager->queueData(SyncManager::DataType::ActivityEvent, sessionId, data);
}

bool ActivityTrackerClient::recordSystemMetrics(float cpuUsage, float gpuUsage, float ramUsage)
{
    QUuid sessionId = this->sessionId();
    if (sessionId.isNull()) {
        LOG_WARNING("Cannot record system metrics: no active session");
        return false;
    }

    QJsonObject data;
    data["cpu_usage"] = cpuUsage;
    data["gpu_usage"] = gpuUsage;
    data["memory_usage"] = ramUsage;

    return m_syncManager->queueData(SyncManager::DataType::SystemMetrics, sessionId, data);
}

bool ActivityTrackerClient::checkAndRegisterMachine() {
    if (!m_apiManager || !m_sessionManager) {
        LOG_ERROR("API Manager or Session Manager not initialized");
        return false;
    }

    LOG_INFO("Checking machine registration status");

    // First try to get machine info
    if (!m_machineId.isEmpty()) {
        QJsonObject machineResponse;
        bool machineExists = m_apiManager->getMachine(m_machineId, machineResponse);

        if (machineExists) {
            LOG_INFO("Machine already registered with ID: " + m_machineId);
            return true;
        }
    }

    // Machine not found, register it
    LOG_INFO("Machine not registered, attempting registration");

    QString hostname = QHostInfo::localHostName();
    QString osName = QSysInfo::prettyProductName();

    QJsonObject machineData;
    bool registered = m_sessionManager->registerMachine(
        hostname,
        osName,
        QSysInfo::machineUniqueId(),
        "", // Mac address can be obtained if needed
        QSysInfo::currentCpuArchitecture(),
        "", // GPU info would need to be collected
        0,  // RAM size would need to be collected
        "", // IP not needed
        &machineData
    );

    // we should consider registration successful when we get machine data back
    if (machineData.contains("id")) {
        // Update machine ID to match what was registered on the server
        QString newMachineId = machineData["id"].toString();
        LOG_INFO("Machine registered successfully with ID: " + newMachineId);

        // Update our stored machine ID if different
        if (newMachineId != m_machineId) {
            LOG_INFO("Updating machine ID from " + m_machineId + " to " + newMachineId);

            // IMPORTANT: Update directly without triggering signals
            m_machineId = newMachineId;

            // Update config without triggering signals
            if (m_configManager) {
                // Temporarily disconnect signal-slot connections
                disconnect(m_configManager, &ConfigManager::machineIdChanged,
                          this, &ActivityTrackerClient::onMachineIdChanged);
                disconnect(m_configManager, &ConfigManager::configChanged,
                          this, &ActivityTrackerClient::onConfigChanged);

                // Update the configuration
                m_configManager->setMachineId(newMachineId);

                // Reconnect signals
                connect(m_configManager, &ConfigManager::machineIdChanged,
                       this, &ActivityTrackerClient::onMachineIdChanged);
                connect(m_configManager, &ConfigManager::configChanged,
                       this, &ActivityTrackerClient::onConfigChanged);

                LOG_INFO("Machine ID updated in configuration without triggering signals");
            }

            if (m_sessionManager) {
                // Update SessionManager with the new machine ID directly
                m_sessionManager->updateMachineId(newMachineId);
            }
        }
        return true;
    }

    LOG_ERROR("Failed to register machine");
    return false;
}

QString ActivityTrackerClient::getAppId(const QString &appName, const QString &executablePath)
{
    // Check if we have a monitor manager with app cache
    if (!m_monitorManager || !m_monitorManager->appCache()) {
        LOG_WARNING("No app cache available to get app ID");
        return QString();
    }

    // First check if app is in cache
    QString appId = m_monitorManager->appCache()->findAppId(executablePath);

    if (appId.isEmpty()) {
        // App not in cache, register it
        appId = m_monitorManager->appCache()->registerApplication(appName, executablePath);
    }

    return appId;
}

bool ActivityTrackerClient::authenticate(const QString& username, const QString& machineId)
{
    if (!m_apiManager) {
        LOG_ERROR("Cannot authenticate: APIManager not initialized");
        return false;
    }

    QJsonObject authResponse;
    return m_apiManager->authenticate(username, machineId, authResponse);
}

bool ActivityTrackerClient::setAuthToken(const QString& token)
{
    if (!m_apiManager) {
        LOG_ERROR("Cannot set auth token: APIManager not initialized");
        return false;
    }

    return m_apiManager->setAuthToken(token);
}

