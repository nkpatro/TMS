#include "SyncManager.h"
#include "APIManager.h"
#include "SessionManager.h"
#include "logger/logger.h"
#include <QHostInfo>
#include <QSysInfo>
#include <QNetworkInterface>

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
    
    // First, ensure server connectivity
    bool isConnected = false;
    if (!m_offlineMode && m_sessionManager->checkServerConnection(isConnected) && isConnected) {
        // We're online, use server API to manage session
        return m_sessionManager->createOrReopenSession(date, sessionId, sessionStart, isNewSession);
    } else {
        // We're offline, create a local session
        LOG_WARNING("Operating in offline mode, creating local session");
        
        // Generate a temporary UUID for offline session
        sessionId = QUuid::createUuid();
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
            case SessionEvent:
                if (!sessionEvents.contains(item.sessionId)) {
                    sessionEvents[item.sessionId] = QJsonArray();
                }
                sessionEvents[item.sessionId].append(item.data);
                break;
                
            case ActivityEvent:
                if (!activityEvents.contains(item.sessionId)) {
                    activityEvents[item.sessionId] = QJsonArray();
                }
                activityEvents[item.sessionId].append(item.data);
                break;
                
            case SystemMetrics:
                if (!systemMetrics.contains(item.sessionId)) {
                    systemMetrics[item.sessionId] = QJsonArray();
                }
                systemMetrics[item.sessionId].append(item.data);
                break;
                
            case AppUsage:
            case AfkPeriod:
                // These are handled individually by the SessionManager
                // Just remove from queue here
                break;
        }
        
        processed++;
        m_dataQueue.dequeue();
    }
    
    locker.unlock();
    
    // Second pass: send batched data
    QMap<QUuid, QJsonArray>::const_iterator i;
    
    // Process session events
    for (i = sessionEvents.constBegin(); i != sessionEvents.constEnd(); ++i) {
        if (!sendBatchedData(i.key(), i.value(), QJsonArray(), QJsonArray())) {
            success = false;
        }
    }
    
    // Process activity events
    for (i = activityEvents.constBegin(); i != activityEvents.constEnd(); ++i) {
        if (!sendBatchedData(i.key(), QJsonArray(), i.value(), QJsonArray())) {
            success = false;
        }
    }
    
    // Process system metrics
    for (i = systemMetrics.constBegin(); i != systemMetrics.constEnd(); ++i) {
        if (!sendBatchedData(i.key(), QJsonArray(), QJsonArray(), i.value())) {
            success = false;
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
    
    LOG_DEBUG(QString("Sending batched data for session %1").arg(sessionId.toString()));
    
    QJsonObject batchData;
    batchData["session_id"] = sessionId.toString();
    
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
    bool success = m_apiManager->processSessionBatch(sessionId, batchData, responseData);
    
    if (!success) {
        LOG_ERROR(QString("Failed to send batched data for session %1").arg(sessionId.toString()));
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
    
    if (m_apiManager->getMachinesByName(hostname, machineResponse)) {
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