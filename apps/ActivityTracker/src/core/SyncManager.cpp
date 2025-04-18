#include "SyncManager.h"

#include <QDir>

#include "APIManager.h"
#include "SessionManager.h"
#include "logger/logger.h"
#include <QHostInfo>
#include <QJsonDocument>
#include <QSysInfo>
#include <QNetworkInterface>

#include "src/service/MultiUserManager.h"

SyncManager::SyncManager(APIManager* apiManager, SessionManager* sessionManager, QObject *parent)
    : QObject(parent)
    , m_apiManager(apiManager)
    , m_sessionManager(sessionManager)
    , m_offlineMode(false)
    , m_isRunning(false)
    , m_initialized(false)
    , m_maxQueueSize(1000)
    , m_syncInterval(60000)
{
    // Set up sync timer
    connect(&m_syncTimer, &QTimer::timeout, this, &SyncManager::onSyncTimerTriggered);
    
    // Set up connection check timer
    connect(&m_connectionCheckTimer, &QTimer::timeout, this, &SyncManager::checkConnection);
    m_connectionCheckTimer.setInterval(30000);  // 30 seconds
}

SyncManager::~SyncManager()
{
    if (m_isRunning) {
        stop();
    }
    
    // Process any remaining queued data
    processPendingQueue();
}

bool SyncManager::initialize(int syncIntervalMs, int maxQueueSize)
{
    LOG_INFO(QString("Initializing SyncManager (sync interval: %1ms, max queue: %2)")
             .arg(syncIntervalMs)
             .arg(maxQueueSize));
    
    if (!m_apiManager || !m_sessionManager) {
        LOG_ERROR("API Manager or Session Manager not initialized");
        return false;
    }
    
    m_syncInterval = syncIntervalMs;
    m_maxQueueSize = maxQueueSize;
    
    // Set up timers with configured intervals
    m_syncTimer.setInterval(m_syncInterval);
    
    m_initialized = true;
    return true;
}

bool SyncManager::start()
{
    if (m_isRunning) {
        LOG_WARNING("SyncManager is already running");
        return true;
    }
    
    LOG_INFO("Starting SyncManager");
    
    if (!m_initialized) {
        LOG_ERROR("SyncManager not initialized");
        return false;
    }
    
    // Initial connection check
    checkConnection();
    
    // Start timers if applicable
    if (m_syncInterval > 0) {
        m_syncTimer.start();
    }
    
    m_connectionCheckTimer.start();
    m_isRunning = true;
    
    LOG_INFO("SyncManager started successfully");
    return true;
}

bool SyncManager::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("SyncManager is not running");
        return true;
    }
    
    LOG_INFO("Stopping SyncManager");
    
    // Stop timers
    m_syncTimer.stop();
    m_connectionCheckTimer.stop();
    
    // Process any pending data before stopping
    processPendingQueue();
    
    m_isRunning = false;
    
    LOG_INFO("SyncManager stopped successfully");
    return true;
}

bool SyncManager::createOrReopenSession(const QDate& date, QUuid& sessionId, QDateTime& sessionStart, bool& isNewSession)
{
    LOG_INFO(QString("Creating or reopening session for date: %1").arg(date.toString()));
    
    if (!m_initialized || !m_apiManager || !m_sessionManager) {
        LOG_ERROR("API Manager or Session Manager not initialized");
        return false;
    }
    
    // Get the current user from SessionManager
    QString currentUser = m_sessionManager->getUsername();

    // Check if we have a MultiUserManager available
    MultiUserManager* multiUserManager = m_sessionManager->getMultiUserManager();

    // First, ensure server connectivity
    bool isConnected = false;
    if (!m_offlineMode && m_sessionManager->checkServerConnection(isConnected) && isConnected) {
        // We're online, try to use server API

        // Check if we're authenticated for the current user
        bool isAuthenticated = false;

        if (multiUserManager) {
            // Use MultiUserManager to check for existing authentication
            isAuthenticated = multiUserManager->hasUserAuthToken(currentUser);

            if (isAuthenticated) {
                // Set the token in the APIManager
                QString token = multiUserManager->getUserAuthToken(currentUser);
                if (!token.isEmpty()) {
                    m_apiManager->setAuthToken(token);
                    isAuthenticated = true;
                    LOG_INFO(QString("Using existing auth token for user '%1'").arg(currentUser));
                } else {
                    isAuthenticated = false;
                }
            }
        } else {
            // Fall back to API Manager check
            isAuthenticated = m_apiManager->isAuthenticated();
        }

        if (!isAuthenticated) {
            LOG_INFO("Not authenticated, attempting authentication");

            // Get machineId from the session manager
            QString machineId = m_sessionManager->getMachineId();

            if (currentUser.isEmpty() || machineId.isEmpty()) {
                LOG_ERROR("Username or machineId not set");
                m_offlineMode = true;
                emit connectionStateChanged(false);

                // Create temporary offline session
                // sessionId = QUuid::createUuid();
                sessionStart = QDateTime::currentDateTime();
                isNewSession = true;
                return true;
            }

            // Try to authenticate, preferably through MultiUserManager
            bool authSuccess = false;

            if (multiUserManager) {
                authSuccess = multiUserManager->authenticateUser(currentUser, machineId, m_apiManager);
            } else {
                // Fall back to direct authentication
                QJsonObject responseData;
                authSuccess = m_apiManager->authenticate(currentUser, machineId, responseData);

                // Since we're not using MultiUserManager, we need to store the token somewhere
                // It's already stored in the APIManager from the authenticate call
            }

            if (!authSuccess) {
                LOG_ERROR("Authentication failed");
                m_offlineMode = true;
                emit connectionStateChanged(false);

                // Create temporary offline session
                // sessionId = QUuid::createUuid();
                sessionStart = QDateTime::currentDateTime();
                isNewSession = true;
                return true;
            }

            LOG_INFO("Authentication successful");
        }

        // We're authenticated, create or find session
        return m_sessionManager->createOrReopenSession(date, sessionId, sessionStart, isNewSession);
    } else {
        // We're offline, create a local session
        LOG_WARNING("Operating in offline mode, creating local session");
        
        // Generate a temporary UUID for offline session
        // sessionId = QUuid::createUuid();
        sessionStart = QDateTime::currentDateTime();
        isNewSession = true;
        
        m_offlineMode = true;
        emit connectionStateChanged(false);
        
        return true;
    }
}

bool SyncManager::closeSession(const QUuid& sessionId)
{
    LOG_INFO(QString("Closing session: %1").arg(sessionId.toString()));
    
    if (!m_initialized || !m_apiManager || !m_sessionManager) {
        LOG_ERROR("API Manager or Session Manager not initialized");
        return false;
    }
    
    // Process any pending data first
    processPendingQueue();
    
    // Close the session through session manager
    return m_sessionManager->closeSession(sessionId);
}

bool SyncManager::queueData(DataType type, const QUuid& sessionId, const QJsonObject& data, const QDateTime& timestamp)
{
    if (sessionId.isNull()) {
        LOG_ERROR("Cannot queue data with null session ID");
        return false;
    }
    
    QueuedData queuedData;
    queuedData.type = type;
    queuedData.sessionId = sessionId;
    queuedData.data = data;
    queuedData.timestamp = timestamp.isValid() ? timestamp : QDateTime::currentDateTime();
    queuedData.retryCount = 0;
    
    // If sync interval is 0, send immediately
    if (m_syncInterval <= 0 && !m_offlineMode) {
        QMutexLocker locker(&m_queueMutex);
        m_dataQueue.enqueue(queuedData);
        locker.unlock();
        
        LOG_DEBUG("Sync interval is 0, processing data immediately");
        return processPendingQueue(1);
    }
    
    // Otherwise add to queue
    QMutexLocker locker(&m_queueMutex);
    m_dataQueue.enqueue(queuedData);
    
    int newSize = m_dataQueue.size();
    emit queueSizeChanged(newSize);
    
    // If queue is getting too large, process it
    if (newSize >= m_maxQueueSize) {
        LOG_INFO(QString("Queue reached threshold (%1), processing").arg(m_maxQueueSize));
        locker.unlock();
        return processPendingQueue();
    }
    
    return true;
}

bool SyncManager::processPendingQueue(int maxItems)
{
    if (!m_initialized || !m_apiManager || !m_sessionManager) {
        LOG_ERROR("API Manager or Session Manager not initialized");
        return false;
    }
    
    // If we're offline, don't try to process the queue
    if (m_offlineMode) {
        LOG_WARNING("In offline mode, not processing queue");
        return false;
    }
    
    // Ensure we're authenticated before processing queue
    if (!m_apiManager->isAuthenticated()) {
        LOG_INFO("Not authenticated, attempting authentication before processing queue");

        // Get username and machineId from the session manager
        QString username = m_sessionManager->getUsername();
        QString machineId = m_sessionManager->getMachineId();

        if (username.isEmpty() || machineId.isEmpty()) {
            LOG_ERROR("Username or machineId not set, cannot authenticate");
            m_offlineMode = true;
            emit connectionStateChanged(false);
            return false;
        }

        // Try to authenticate
        QJsonObject responseData;
        bool success = m_apiManager->authenticate(username, machineId, responseData);

        if (!success) {
            LOG_ERROR("Authentication failed, cannot process queue");
            m_offlineMode = true;
            emit connectionStateChanged(false);
            return false;
        }

        LOG_INFO("Authentication successful, proceeding with queue processing");
    }

    QMutexLocker locker(&m_queueMutex);

    if (m_dataQueue.isEmpty()) {
        return true;
    }

    LOG_INFO(QString("Processing pending queue (items: %1, max: %2)")
             .arg(m_dataQueue.size())
             .arg(maxItems > 0 ? QString::number(maxItems) : "all"));

    // Group data by session and type for batch processing
    QMap<QUuid, QJsonArray> sessionEvents;
    QMap<QUuid, QJsonArray> activityEvents;
    QMap<QUuid, QJsonArray> systemMetrics;

    int processed = 0;
    bool success = true;

    // First pass: organize data into batches
    while (!m_dataQueue.isEmpty() && (maxItems <= 0 || processed < maxItems)) {
        const QueuedData& item = m_dataQueue.head();

        switch (item.type) {
            case DataType::SessionEvent:
                if (!sessionEvents.contains(item.sessionId)) {
                    sessionEvents[item.sessionId] = QJsonArray();
                }
                sessionEvents[item.sessionId].append(item.data);
                break;

            case DataType::ActivityEvent:
                if (!activityEvents.contains(item.sessionId)) {
                    activityEvents[item.sessionId] = QJsonArray();
                }
                activityEvents[item.sessionId].append(item.data);
                break;

            case DataType::SystemMetrics:
                if (!systemMetrics.contains(item.sessionId)) {
                    systemMetrics[item.sessionId] = QJsonArray();
                }
                systemMetrics[item.sessionId].append(item.data);
                break;

            case DataType::AppUsage: {
                    // Handle app usage items individually
                    locker.unlock();
                    bool itemSuccess = false;

                    itemSuccess = processAppUsageData(item);

                    locker.relock();
                    break;
            }

            case DataType::AfkPeriod: {
                // Handle AFK period items individually
                locker.unlock();
                bool itemSuccess = false;

                if (item.data.contains("action") && item.data["action"].toString() == "end") {
                    // End AFK period request
                    QUuid afkId = QUuid(item.data["afk_id"].toString());
                    QJsonObject response;
                    itemSuccess = m_apiManager->endAfkPeriod(afkId, item.data, response);
                    emit dataProcessed(DataType::AfkPeriod, item.sessionId, itemSuccess);
                } else {
                    // Start AFK period request
                    QJsonObject response;
                    itemSuccess = m_apiManager->startAfkPeriod(item.data, response);
                    emit dataProcessed(DataType::AfkPeriod, item.sessionId, itemSuccess);
                }

                if (!itemSuccess) {
                    success = false;
                }

                locker.relock();
                break;
            }
        }

        processed++;
        m_dataQueue.dequeue();
    }

    // Update queue size after dequeuing
    int newQueueSize = m_dataQueue.size();
    locker.unlock();
    emit queueSizeChanged(newQueueSize);

    // Second pass: send batched data
    // Process session events
    for (auto it = sessionEvents.constBegin(); it != sessionEvents.constEnd(); ++it) {
        QUuid sessionId = it.key();
        QJsonArray events = it.value();

        if (!events.isEmpty()) {
            bool batchSuccess = sendBatchedData(sessionId, events, QJsonArray(), QJsonArray());
            if (!batchSuccess) {
                success = false;
            }
            emit dataProcessed(DataType::SessionEvent, sessionId, batchSuccess);
        }
    }

    // Process activity events
    for (auto it = activityEvents.constBegin(); it != activityEvents.constEnd(); ++it) {
        QUuid sessionId = it.key();
        QJsonArray events = it.value();

        if (!events.isEmpty()) {
            bool batchSuccess = sendBatchedData(sessionId, QJsonArray(), events, QJsonArray());
            if (!batchSuccess) {
                success = false;
            }
            emit dataProcessed(DataType::ActivityEvent, sessionId, batchSuccess);
        }
    }

    // Process system metrics
    for (auto it = systemMetrics.constBegin(); it != systemMetrics.constEnd(); ++it) {
        QUuid sessionId = it.key();
        QJsonArray metrics = it.value();

        if (!metrics.isEmpty()) {
            bool batchSuccess = sendBatchedData(sessionId, QJsonArray(), QJsonArray(), metrics);
            if (!batchSuccess) {
                success = false;
            }
            emit dataProcessed(DataType::SystemMetrics, sessionId, batchSuccess);
        }
    }
    
    m_lastSyncTime = QDateTime::currentDateTime();
    emit syncCompleted(success, processed);
    
    LOG_INFO(QString("Processed %1 items from queue, success: %2").arg(processed).arg(success ? "true" : "false"));
    return success;
}

void SyncManager::checkConnection()
{
    if (!m_initialized || !m_apiManager || !m_sessionManager) {
        return;
    }
    
    LOG_DEBUG("Checking server connection");
    
    bool isConnected = false;
    if (m_sessionManager->checkServerConnection(isConnected)) {
        if (isConnected && m_offlineMode) {
            LOG_INFO("Server connection restored, exiting offline mode");
            m_offlineMode = false;
            emit connectionStateChanged(true);
            
            // Process any pending data
            processPendingQueue();
        } else if (!isConnected && !m_offlineMode) {
            LOG_WARNING("Server connection lost, entering offline mode");
            m_offlineMode = true;
            emit connectionStateChanged(false);
        }
    }
    
    m_lastConnectionCheck = QDateTime::currentDateTime();
}

void SyncManager::forceSyncNow()
{
    LOG_INFO("Forcing immediate data sync");
    
    checkConnection();
    
    if (!m_offlineMode) {
        processPendingQueue();
    }
}

void SyncManager::onSyncTimerTriggered()
{
    LOG_DEBUG("Sync timer triggered");
    
    checkConnection();
    
    if (!m_offlineMode) {
        processPendingQueue();
    }
}

bool SyncManager::batchAndProcessData()
{
    // Note: This is a simpler version of processPendingQueue
    // It's meant to be called for immediate data processing
    
    return processPendingQueue();
}

bool SyncManager::sendBatchedData(const QUuid& sessionId, const QJsonArray& sessionEvents,
                                const QJsonArray& activityEvents, const QJsonArray& systemMetrics)
{
    if (sessionId.isNull()) {
        LOG_ERROR("Cannot send batched data with null session ID");
        return false;
    }

    if (sessionEvents.isEmpty() && activityEvents.isEmpty() && systemMetrics.isEmpty()) {
        // Nothing to send
        return true;
    }

    // Format the session ID correctly: remove curly braces
    QString cleanSessionId = sessionId.toString().remove('{').remove('}');

    LOG_DEBUG(QString("Sending batched data for session %1").arg(cleanSessionId));

    // Prepare the batch data with clean session ID
    QJsonObject batchData;
    batchData["session_id"] = cleanSessionId;

    if (!sessionEvents.isEmpty()) {
        batchData["session_events"] = sessionEvents;
    }

    if (!activityEvents.isEmpty()) {
        batchData["activity_events"] = activityEvents;
    }

    if (!systemMetrics.isEmpty()) {
        batchData["system_metrics"] = systemMetrics;
    }

    QJsonObject responseData;

    // Use the general batch endpoint directly instead of trying session-specific endpoint first
    bool success = m_apiManager->processBatch(batchData, responseData);

    if (success) {
        LOG_DEBUG(QString("Successfully sent batched data for session %1").arg(cleanSessionId));
        // Reset consecutive failures counter
        m_consecutiveFailures = 0;
    } else {
        LOG_ERROR(QString("Failed to send batched data for session %1").arg(cleanSessionId));

        // Count consecutive failures for potential fallback to offline mode
        m_consecutiveFailures++;

        // Check if we should switch to offline mode after multiple failures
        if (m_consecutiveFailures >= MAX_CONSECUTIVE_FAILURES && !m_offlineMode) {
            LOG_WARNING(QString("Switching to offline mode after %1 consecutive failures").arg(m_consecutiveFailures));
            m_offlineMode = true;
            emit connectionStateChanged(false);
        }

        // Store the failed batch for later retry if we have persistence enabled
        if (m_enablePersistence) {
            storeFailedBatchForRetry(sessionId, batchData);
        }
    }
    
    return success;
}

bool SyncManager::registerMachine(const QString& hostname, QString& machineId)
{
    LOG_INFO(QString("Registering machine: %1").arg(hostname));
    
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("API Manager not initialized");
        return false;
    }
    
    // First try to find existing machine by hostname
    QJsonObject query;
    query["hostname"] = hostname;
    QJsonObject machineResponse;
    
    if (m_apiManager->getMachineByName(hostname, machineResponse)) {
        if (machineResponse.contains("machines") && machineResponse["machines"].isArray()) {
            QJsonArray machines = machineResponse["machines"].toArray();
            if (!machines.isEmpty()) {
                // Found existing machine
                machineId = machines.first().toObject()["machine_id"].toString();
                LOG_INFO(QString("Found existing machine with ID: %1").arg(machineId));
                return true;
            }
        }
    }
    
    // Need to register a new machine
    QJsonObject machineData;
    machineData["hostname"] = hostname;
    machineData["os_name"] = QSysInfo::prettyProductName();
    machineData["os_version"] = QSysInfo::productVersion();
    machineData["cpu_info"] = QSysInfo::currentCpuArchitecture();
    
    // Get MAC address (optional)
    QString macAddress;
    foreach (const QNetworkInterface &netInterface, QNetworkInterface::allInterfaces()) {
        // Skip loopback and non-available interfaces
        if (netInterface.flags().testFlag(QNetworkInterface::IsLoopBack) ||
            !netInterface.flags().testFlag(QNetworkInterface::IsUp) ||
            !netInterface.flags().testFlag(QNetworkInterface::IsRunning)) {
            continue;
        }
        
        macAddress = netInterface.hardwareAddress();
        if (!macAddress.isEmpty()) {
            break;
        }
    }
    
    if (!macAddress.isEmpty()) {
        machineData["mac_address"] = macAddress;
    }
    
    QJsonObject responseData;
    if (m_apiManager->registerMachine(machineData, responseData)) {
        if (responseData.contains("machine_id")) {
            machineId = responseData["machine_id"].toString();
            LOG_INFO(QString("Registered new machine with ID: %1").arg(machineId));
            return true;
        }
    }
    
    LOG_ERROR("Failed to register machine");
    return false;
}

bool SyncManager::authenticateUser(const QString& username, const QString& machineId)
{
    LOG_INFO(QString("Authenticating user: %1 on machine: %2").arg(username, machineId));
    
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("API Manager not initialized");
        return false;
    }
    
    QJsonObject authData;
    authData["username"] = username;
    authData["machine_id"] = machineId;
    
    QJsonObject responseData;
    bool success = m_apiManager->authenticate(username, machineId, responseData);
    
    if (success) {
        LOG_INFO("User authenticated successfully");
    } else {
        LOG_ERROR("User authentication failed");
    }
    
    return success;
}

void SyncManager::storeFailedBatchForRetry(const QUuid& sessionId, const QJsonObject& batchData)
{
    if (!m_enablePersistence) return;

    // Create a unique filename based on timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    QString filename = QDir::tempPath() + "/activity_tracker_" +
                      sessionId.toString().remove('{').remove('}') + "_" +
                      timestamp + ".json";

    // Serialize the batch data to JSON
    QJsonDocument doc(batchData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

    // Write to file
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonData);
        file.close();
        LOG_INFO(QString("Stored failed batch for later retry: %1").arg(filename));
    } else {
        LOG_ERROR(QString("Failed to save batch data for retry: %1").arg(file.errorString()));
    }
}

bool SyncManager::processAppUsageData(const QueuedData& item) {
    if (!m_apiManager) {
        LOG_ERROR("API Manager not initialized for processing app usage data");
        return false;
    }

    // Extract session ID - critical for session-based endpoints
    QString sessionId;
    if (item.data.contains("session_id")) {
        sessionId = item.data["session_id"].toString().remove('{').remove('}');
    } else if (!item.sessionId.isNull()) {
        sessionId = item.sessionId.toString().remove('{').remove('}');
    } else {
        LOG_ERROR("Cannot process app usage: no session ID available");
        return false;
    }

    // Ensure we have valid session ID
    if (sessionId.isEmpty() || sessionId == "00000000-0000-0000-0000-000000000000") {
        LOG_ERROR("Cannot process app usage: invalid session ID");
        return false;
    }

    // Create a copy of the data to ensure session_id is included
    QJsonObject data = item.data;
    data["session_id"] = sessionId;

    bool success = false;

    if (data.contains("action") && data["action"].toString() == "end") {
        // This is an end app usage request
        if (!data.contains("usage_id") || data["usage_id"].toString().isEmpty()) {
            LOG_ERROR("Cannot end app usage: usage_id is missing");
            return false;
        }

        QString usageId = data["usage_id"].toString().remove('{').remove('}');

        // Skip processing for null UUIDs
        if (usageId == "00000000-0000-0000-0000-000000000000") {
            LOG_WARNING("Skipping end app usage with null UUID");
            return true; // Return true to remove this item from queue
        }

        QJsonObject response;
        success = m_apiManager->endAppUsage(QUuid(usageId), data, response);
    } else {
        // This is a start app usage request
        // Verify we have a valid app_id
        if (!data.contains("app_id") || data["app_id"].toString().isEmpty() ||
            data["app_id"].toString() == "00000000-0000-0000-0000-000000000000") {
            LOG_WARNING("Skipping start app usage with missing or invalid app_id");
            return true; // Return true to remove this item from queue
        }

        QJsonObject response;
        success = m_apiManager->startAppUsage(data, response);
    }

    return success;
}