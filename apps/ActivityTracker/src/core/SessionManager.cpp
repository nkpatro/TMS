#include "SessionManager.h"
#include "logger/logger.h"
#include <QJsonArray>

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
    , m_apiManager(nullptr)
    , m_maxQueueSize(200)
    , m_initialized(false)
    , m_multiUserManager(nullptr)
{
}

SessionManager::~SessionManager()
{
    // Attempt to process any remaining queued data on shutdown
    processQueue();
}

bool SessionManager::initialize(APIManager *apiManager, const QString &username, const QString &machineId)
{
    LOG_INFO("Initializing SessionManager");

    if (m_initialized) {
        LOG_WARNING("SessionManager already initialized");
        return true;
    }

    if (!apiManager) {
        LOG_ERROR("API Manager is null");
        return false;
    }

    m_apiManager = apiManager;
    m_username = username;
    m_machineId = machineId;

    m_initialized = true;
    LOG_INFO("SessionManager initialized successfully");
    return true;
}

bool SessionManager::createOrReopenSession(const QDate &date, QUuid &sessionId, QDateTime &sessionStart, bool &isNewSession)
{
    LOG_INFO(QString("Creating or reopening session for date: %1").arg(date.toString()));

    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    // Check if we already have a session for this date in our local cache
    if (m_sessionsByDate.contains(date)) {
        sessionId = m_sessionsByDate[date];

        // Check if the session is still valid on the server
        QJsonObject sessionData;
        if (m_apiManager->getSession(sessionId, sessionData)) {
            // Session exists, return it
            sessionStart = QDateTime::fromString(sessionData["login_time"].toString(), Qt::ISODate);
            isNewSession = false;

            LOG_INFO(QString("Found existing session for date: %1, ID: %2").arg(
                date.toString(), sessionId.toString()));

            return true;
        }
    }

    // No session found locally or session no longer valid on server
    // Try to get any active session for this machine
    QJsonObject activeSessionData;
    if (m_apiManager->getActiveSession(m_machineId, activeSessionData)) {
        // Check if this session is for today
        QDateTime loginTime = QDateTime::fromString(
            activeSessionData["login_time"].toString(), Qt::ISODate);

        if (loginTime.date() == date) {
            sessionId = QUuid(activeSessionData["session_id"].toString());
            sessionStart = loginTime;
            isNewSession = false;

            // Update our local cache
            m_sessionsByDate[date] = sessionId;

            LOG_INFO(QString("Found active session for date: %1, ID: %2").arg(
                date.toString(), sessionId.toString()));

            return true;
        }
    }

    // No session found, create a new one
    QJsonObject newSessionData;
    newSessionData["username"] = m_username;
    newSessionData["machine_id"] = m_machineId;

    QJsonObject sessionData;
    if (m_apiManager->createSession(newSessionData, sessionData)) {
        sessionId = QUuid(sessionData["session_id"].toString());
        sessionStart = QDateTime::fromString(sessionData["login_time"].toString(), Qt::ISODate);
        isNewSession = true;

        // Update our local cache
        m_sessionsByDate[date] = sessionId;

        LOG_INFO(QString("Created new session for date: %1, ID: %2").arg(
            date.toString(), sessionId.toString()));

        return true;
    }

    LOG_ERROR("Failed to create or reopen session");
    return false;
}

bool SessionManager::closeSession(const QUuid &sessionId)
{
    LOG_INFO(QString("Closing session: %1").arg(sessionId.toString()));

    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    // End any active AFK period
    if (m_activeAfkPeriodId != QUuid()) {
        endAfkPeriod(sessionId);
    }

    // Process any pending data for this session
    processQueue();

    // Send request to end session
    QJsonObject sessionData;
    bool success = m_apiManager->endSession(sessionId, sessionData);

    if (success) {
        LOG_INFO(QString("Session closed successfully: %1").arg(sessionId.toString()));

        // Remove from our date cache
        for (auto it = m_sessionsByDate.begin(); it != m_sessionsByDate.end(); ++it) {
            if (it.value() == sessionId) {
                m_sessionsByDate.erase(it);
                break;
            }
        }
    } else {
        LOG_ERROR(QString("Failed to close session: %1").arg(sessionId.toString()));
    }

    return success;
}

bool SessionManager::recordSessionEvent(const QUuid &sessionId, const QString &eventType, const QJsonObject &eventData)
{
    LOG_DEBUG(QString("Recording session event: %1 for session %2").arg(eventType, sessionId.toString()));

    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    QJsonObject data = eventData;
    data["event_type"] = eventType;
    data["session_id"] = sessionId.toString();
    data["event_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Update our local cache of events
    QDateTime eventTime = QDateTime::currentDateTime();
    m_lastEventTimes[sessionId] = eventTime;

    // Special handling for logout and lock events
    if (eventType == "logout") {
        m_lastSessionLogoutTimes[sessionId] = eventTime;
    } else if (eventType == "lock") {
        m_lastSessionLockTimes[sessionId] = eventTime;
    }

    // Queue event for sending to server
    return addToPendingQueue(DataType::SessionEvent, sessionId, data, eventTime);
}

bool SessionManager::recordLoginEvent(const QUuid &sessionId, const QDateTime &loginTime, const QJsonObject &eventData)
{
    LOG_DEBUG(QString("Recording login event at %1 for session %2").arg(
        loginTime.toString(), sessionId.toString()));

    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    QJsonObject data = eventData;
    data["event_type"] = "login";
    data["session_id"] = sessionId.toString();
    data["event_time"] = loginTime.toString(Qt::ISODate);

    // Update our local cache
    m_lastEventTimes[sessionId] = loginTime;

    // Queue event for sending to server
    return addToPendingQueue(DataType::SessionEvent, sessionId, data, loginTime);
}

bool SessionManager::recordMissingLogoutEvent(const QUuid &sessionId, const QDateTime &logoutTime, const QJsonObject &eventData)
{
    LOG_DEBUG(QString("Recording missing logout event at %1 for session %2").arg(
        logoutTime.toString(), sessionId.toString()));

    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    QJsonObject data = eventData;
    data["event_type"] = "logout";
    data["session_id"] = sessionId.toString();
    data["event_time"] = logoutTime.toString(Qt::ISODate);

    // Update our local cache
    m_lastEventTimes[sessionId] = logoutTime;
    m_lastSessionLogoutTimes[sessionId] = logoutTime;

    // Queue event for sending to server
    return addToPendingQueue(DataType::SessionEvent, sessionId, data, logoutTime);
}

bool SessionManager::recordActivityEvent(const QUuid &sessionId, const QString &eventType, const QJsonObject &eventData)
{
    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    QJsonObject data = eventData;
    data["event_type"] = eventType;
    data["session_id"] = sessionId.toString();
    data["event_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Update our local cache
    m_lastEventTimes[sessionId] = QDateTime::currentDateTime();

    // Queue event for sending to server
    return addToPendingQueue(DataType::ActivityEvent, sessionId, data);
}

bool SessionManager::startAppUsage(const QUuid &sessionId, const QString &appName, const QString &windowTitle,
                                  const QString &executablePath, QUuid &usageId)
{
    LOG_DEBUG(QString("Starting app usage tracking for %1 in session %2").arg(
        appName, sessionId.toString()));

    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    // Prepare app usage data
    QJsonObject data;
    // Add session ID - format without curly braces
    data["session_id"] = sessionId.toString().remove('{').remove('}');
    data["app_name"] = appName;
    data["window_title"] = windowTitle;
    data["executable_path"] = executablePath;
    data["start_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Send request to start app usage
    QJsonObject responseData;
    bool success = m_apiManager->startAppUsage(data, responseData);

    if (success && responseData.contains("usage_id")) {
        usageId = QUuid(responseData["usage_id"].toString());

        // Cache the usage ID
        m_appUsageIds[usageId] = sessionId;

        LOG_DEBUG(QString("App usage started: %1 for %2").arg(usageId.toString(), appName));
        return true;
    }

    // If API call failed, queue it for later
    if (!success) {
        LOG_WARNING(QString("Failed to start app usage, queuing: %1").arg(appName));

        // Make sure we're queuing data with the session ID
        if (!data.contains("session_id")) {
            data["session_id"] = sessionId.toString().remove('{').remove('}');
        }

        addToPendingQueue(DataType::AppUsage, sessionId, data);

        // Generate a temporary ID
        usageId = QUuid::createUuid();
        return true;
    }

    return false;
}

bool SessionManager::endAppUsage(const QUuid &usageId)
{
    LOG_DEBUG(QString("Ending app usage: %1").arg(usageId.toString()));

    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    // Find the session ID for this usage
    QUuid sessionId;
    if (m_appUsageIds.contains(usageId)) {
        sessionId = m_appUsageIds[usageId];
    }

    if (sessionId.isNull()) {
        LOG_WARNING("No session ID found for usage ID, cannot end app usage");
        return false;
    }

    // Prepare app usage data
    QJsonObject data;
    data["usage_id"] = usageId.toString().remove('{').remove('}');
    data["session_id"] = sessionId.toString().remove('{').remove('}');
    data["end_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Send request to end app usage
    QJsonObject responseData;
    bool success = m_apiManager->endAppUsage(usageId, data, responseData);

    if (success) {
        // Remove from our cache
        m_appUsageIds.remove(usageId);

        LOG_DEBUG(QString("App usage ended successfully: %1").arg(usageId.toString()));
        return true;
    }

    // If API call failed, queue it for later
    if (!success) {
        LOG_WARNING(QString("Failed to end app usage, queuing: %1").arg(usageId.toString()));
        data["action"] = "end";
        addToPendingQueue(DataType::AppUsage, sessionId, data);

        // We still want to remove it from our tracking
        m_appUsageIds.remove(usageId);
        return true;
    }

    return false;
}

bool SessionManager::recordSystemMetrics(const QUuid &sessionId, float cpuUsage, float gpuUsage, float ramUsage)
{
    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    // Prepare metrics data
    QJsonObject data;
    data["session_id"] = sessionId.toString();
    data["cpu_usage"] = cpuUsage;
    data["gpu_usage"] = gpuUsage;
    data["memory_usage"] = ramUsage;
    data["measurement_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Queue metrics for sending to server
    return addToPendingQueue(DataType::SystemMetrics, sessionId, data);
}

bool SessionManager::startAfkPeriod(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Starting AFK period for session %1").arg(sessionId.toString()));

    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    // Make sure we don't already have an active AFK period
    if (m_activeAfkPeriodId != QUuid()) {
        LOG_WARNING("Attempting to start AFK period when one is already active");
        return false;
    }

    // Prepare AFK data
    QJsonObject data;
    data["session_id"] = sessionId.toString();
    data["start_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Send request to start AFK period
    QJsonObject responseData;
    bool success = m_apiManager->startAfkPeriod(data, responseData);

    if (success && responseData.contains("afk_id")) {
        m_activeAfkPeriodId = QUuid(responseData["afk_id"].toString());

        LOG_DEBUG(QString("AFK period started: %1").arg(m_activeAfkPeriodId.toString()));
        return true;
    }

    // If API call failed, queue it for later
    if (!success) {
        LOG_WARNING("Failed to start AFK period, queuing");
        addToPendingQueue(DataType::AfkPeriod, sessionId, data);

        // Set a temporary ID
        m_activeAfkPeriodId = QUuid::createUuid();
        return true;
    }

    return false;
}

bool SessionManager::endAfkPeriod(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Ending AFK period for session %1").arg(sessionId.toString()));

    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    // Make sure we have an active AFK period
    if (m_activeAfkPeriodId == QUuid()) {
        LOG_WARNING("No active AFK period to end");
        return false;
    }

    // Prepare AFK data
    QJsonObject data;
    data["afk_id"] = m_activeAfkPeriodId.toString();
    data["end_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Send request to end AFK period
    QJsonObject responseData;
    bool success = m_apiManager->endAfkPeriod(m_activeAfkPeriodId, data, responseData);

    // Clear the active AFK period regardless of success
    QUuid oldAfkId = m_activeAfkPeriodId;
    m_activeAfkPeriodId = QUuid();

    if (success) {
        LOG_DEBUG(QString("AFK period ended successfully: %1").arg(oldAfkId.toString()));
        return true;
    }

    // If API call failed, queue it for later
    if (!success) {
        LOG_WARNING(QString("Failed to end AFK period, queuing: %1").arg(oldAfkId.toString()));
        data["action"] = "end";
        addToPendingQueue(DataType::AfkPeriod, sessionId, data);
        return true;
    }

    return false;
}

bool SessionManager::getLastSessionLogoutTime(const QUuid &sessionId, QDateTime &logoutTime)
{
    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    // First check our local cache
    if (m_lastSessionLogoutTimes.contains(sessionId)) {
        logoutTime = m_lastSessionLogoutTimes[sessionId];
        return true;
    }

    // If not in cache, query the server
    QJsonObject params;
    params["session_id"] = sessionId.toString();
    params["event_type"] = "logout";

    QJsonObject sessionEvent;
    if (m_apiManager->getLastSessionEvent(params, sessionEvent)) {
        if (sessionEvent.contains("event_time")) {
            logoutTime = QDateTime::fromString(sessionEvent["event_time"].toString(), Qt::ISODate);

            // Update our cache
            m_lastSessionLogoutTimes[sessionId] = logoutTime;

            return true;
        }
    }

    return false;
}

bool SessionManager::getLastSessionLockTime(const QUuid &sessionId, QDateTime &lockTime)
{
    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    // First check our local cache
    if (m_lastSessionLockTimes.contains(sessionId)) {
        lockTime = m_lastSessionLockTimes[sessionId];
        return true;
    }

    // If not in cache, query the server
    QJsonObject params;
    params["session_id"] = sessionId.toString();
    params["event_type"] = "lock";

    QJsonObject sessionEvent;
    if (m_apiManager->getLastSessionEvent(params, sessionEvent)) {
        if (sessionEvent.contains("event_time")) {
            lockTime = QDateTime::fromString(sessionEvent["event_time"].toString(), Qt::ISODate);

            // Update our cache
            m_lastSessionLockTimes[sessionId] = lockTime;

            return true;
        }
    }

    return false;
}

bool SessionManager::getLastEventTime(const QUuid &sessionId, QDateTime &eventTime)
{
    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    // First check our local cache
    if (m_lastEventTimes.contains(sessionId)) {
        eventTime = m_lastEventTimes[sessionId];
        return true;
    }

    // If not in cache, query the server
    QJsonObject params;
    params["session_id"] = sessionId.toString();

    QJsonObject lastEvent;
    if (m_apiManager->getLastEvent(params, lastEvent)) {
        if (lastEvent.contains("event_time")) {
            eventTime = QDateTime::fromString(lastEvent["event_time"].toString(), Qt::ISODate);

            // Update our cache
            m_lastEventTimes[sessionId] = eventTime;

            return true;
        }
    }

    return false;
}

bool SessionManager::syncPendingData()
{
    LOG_DEBUG("Synchronizing pending data");

    if (!m_initialized) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    return processQueue();
}

bool SessionManager::addToPendingQueue(DataType type, const QUuid &sessionId, const QJsonObject &data,
                                      const QDateTime &timestamp)
{
    PendingData pending;
    pending.type = type;
    pending.sessionId = sessionId;
    pending.data = data;
    pending.timestamp = timestamp.isValid() ? timestamp : QDateTime::currentDateTime();

    m_pendingQueue.enqueue(pending);

    // If queue is getting too large, process it
    if (m_pendingQueue.size() >= m_maxQueueSize) {
        LOG_INFO(QString("Queue reached threshold (%1), processing").arg(m_maxQueueSize));
        processQueue();
    }

    return true;
}

bool SessionManager::processQueue(int maxItems)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("Cannot process queue: SessionManager not initialized or API manager is null");
        return false;
    }

    if (m_pendingQueue.isEmpty()) {
        return true;
    }

    int processed = 0;
    bool success = true;

    // Group similar consecutive items to send in batches
    QMap<QUuid, QJsonArray> sessionEventBatches;
    QMap<QUuid, QJsonArray> activityEventBatches;
    QMap<QUuid, QJsonArray> systemMetricsBatches;

    while (!m_pendingQueue.isEmpty() && (maxItems <= 0 || processed < maxItems)) {
        PendingData item = m_pendingQueue.dequeue();
        processed++;

        bool itemSuccess = false;

        switch (item.type) {
            case DataType::SessionEvent:
                // Add to session event batch
                if (!sessionEventBatches.contains(item.sessionId)) {
                    sessionEventBatches[item.sessionId] = QJsonArray();
                }
                sessionEventBatches[item.sessionId].append(item.data);
                itemSuccess = true;
                break;

            case DataType::ActivityEvent:
                // Add to activity event batch
                if (!activityEventBatches.contains(item.sessionId)) {
                    activityEventBatches[item.sessionId] = QJsonArray();
                }
                activityEventBatches[item.sessionId].append(item.data);
                itemSuccess = true;
                break;

            case DataType::SystemMetrics:
                // Add to system metrics batch
                if (!systemMetricsBatches.contains(item.sessionId)) {
                    systemMetricsBatches[item.sessionId] = QJsonArray();
                }
                systemMetricsBatches[item.sessionId].append(item.data);
                itemSuccess = true;
                break;

            case DataType::AppUsage:
                // Handle app usage - these aren't batched since they need individual responses
                if (item.data.contains("action") && item.data["action"].toString() == "end") {
                    // This is an end app usage request
                    QUuid usageId = QUuid(item.data["usage_id"].toString());
                    QJsonObject response;
                    itemSuccess = m_apiManager->endAppUsage(usageId, item.data, response);
                } else {
                    // This is a start app usage request
                    QJsonObject response;
                    itemSuccess = m_apiManager->startAppUsage(item.data, response);
                }
                break;

            case DataType::AfkPeriod:
                // Handle AFK period - these aren't batched since they need individual responses
                if (item.data.contains("action") && item.data["action"].toString() == "end") {
                    // This is an end AFK period request
                    QUuid afkId = QUuid(item.data["afk_id"].toString());
                    QJsonObject response;
                    itemSuccess = m_apiManager->endAfkPeriod(afkId, item.data, response);
                } else {
                    // This is a start AFK period request
                    QJsonObject response;
                    itemSuccess = m_apiManager->startAfkPeriod(item.data, response);
                }
                break;
        }

        if (!itemSuccess) {
            success = false;
        }
    }

    // Now send all batched data
    for (auto it = sessionEventBatches.begin(); it != sessionEventBatches.end(); ++it) {
        QUuid sessionId = it.key();
        QJsonArray events = it.value();

        QJsonObject batchData;
        batchData["session_id"] = sessionId.toString();
        batchData["session_events"] = events;

        if (!m_apiManager->batchSessionEvents(batchData)) {
            success = false;
        }
    }

    for (auto it = activityEventBatches.begin(); it != activityEventBatches.end(); ++it) {
        QUuid sessionId = it.key();
        QJsonArray events = it.value();

        QJsonObject batchData;
        batchData["session_id"] = sessionId.toString();
        batchData["activity_events"] = events;

        if (!m_apiManager->batchActivityEvents(batchData)) {
            success = false;
        }
    }

    for (auto it = systemMetricsBatches.begin(); it != systemMetricsBatches.end(); ++it) {
        QUuid sessionId = it.key();
        QJsonArray metrics = it.value();

        QJsonObject batchData;
        batchData["session_id"] = sessionId.toString();
        batchData["system_metrics"] = metrics;

        if (!m_apiManager->batchSystemMetrics(batchData)) {
            success = false;
        }
    }

    LOG_INFO(QString("Processed %1 items from queue, success: %2").arg(processed).arg(success));
    return success;
}

bool SessionManager::getSessionStatistics(const QUuid &sessionId, QJsonObject &statsData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Getting statistics for session: %1").arg(sessionId.toString()));

    return m_apiManager->getSessionStats(sessionId, statsData);
}

bool SessionManager::getApplicationUsages(const QUuid &sessionId, bool activeOnly, QJsonObject &usageData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Getting app usages for session: %1").arg(sessionId.toString()));

    return m_apiManager->getAppUsages(sessionId, activeOnly, usageData);
}

bool SessionManager::getTopApplications(const QUuid &sessionId, int limit, QJsonObject &topAppsData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Getting top applications for session: %1").arg(sessionId.toString()));

    return m_apiManager->getTopApps(sessionId, limit, topAppsData);
}

bool SessionManager::getActiveApplications(const QUuid &sessionId, QJsonObject &activeAppsData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Getting active applications for session: %1").arg(sessionId.toString()));

    return m_apiManager->getActiveApps(sessionId, activeAppsData);
}

bool SessionManager::getSessionChain(const QUuid &sessionId, QJsonObject &chainData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Getting session chain for session: %1").arg(sessionId.toString()));

    return m_apiManager->getSessionChain(sessionId, chainData);
}

bool SessionManager::getAllAfkPeriods(const QUuid &sessionId, QJsonObject &afkData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Getting all AFK periods for session: %1").arg(sessionId.toString()));

    return m_apiManager->getAfkPeriods(sessionId, afkData);
}

bool SessionManager::getSystemMetricsAverage(const QUuid &sessionId, QJsonObject &metricsData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Getting average system metrics for session: %1").arg(sessionId.toString()));

    return m_apiManager->getAverageMetrics(sessionId, metricsData);
}

bool SessionManager::getSystemMetricsTimeSeries(const QUuid &sessionId, const QString &metricType, QJsonObject &timeSeriesData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Getting %1 metrics time series for session: %2").arg(metricType, sessionId.toString()));

    return m_apiManager->getMetricsTimeSeries(sessionId, metricType, timeSeriesData);
}

bool SessionManager::detectOrCreateApplication(const QString &appName, const QString &appPath, const QString &appHash, bool isRestricted, bool trackingEnabled, QJsonObject &appData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Detecting or creating application: %1 (%2)").arg(appName, appPath));

    QJsonObject detectData;
    detectData["app_name"] = appName;
    detectData["app_path"] = appPath;

    if (!appHash.isEmpty()) {
        detectData["app_hash"] = appHash;
    }

    detectData["is_restricted"] = isRestricted;
    detectData["tracking_enabled"] = trackingEnabled;

    return m_apiManager->detectApplication(detectData, appData);
}

bool SessionManager::registerMachine(const QString &name, const QString &operatingSystem,
                                    const QString &machineUniqueId, const QString &macAddress,
                                    const QString &cpuInfo, const QString &gpuInfo,
                                    int ramSizeGB, const QString &lastKnownIp,
                                    QJsonObject *machineData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Registering machine: %1").arg(name));

    QJsonObject registerData;
    registerData["name"] = name;
    registerData["operatingSystem"] = operatingSystem;

    if (!machineUniqueId.isEmpty()) {
        registerData["machineUniqueId"] = machineUniqueId;
    }

    if (!macAddress.isEmpty()) {
        registerData["macAddress"] = macAddress;
    }

    if (!cpuInfo.isEmpty()) {
        registerData["cpuInfo"] = cpuInfo;
    }

    if (!gpuInfo.isEmpty()) {
        registerData["gpuInfo"] = gpuInfo;
    }

    if (ramSizeGB > 0) {
        registerData["ramSizeGB"] = ramSizeGB;
    }

    if (!lastKnownIp.isEmpty()) {
        registerData["lastKnownIp"] = lastKnownIp;
    }

    QJsonObject responseData;
    bool success = m_apiManager->registerMachine(registerData, responseData);

    // If caller wants the response data and the call was successful
    if (machineData != nullptr && success) {
        *machineData = responseData;
    }

    return success;
}

bool SessionManager::updateMachineStatus(const QString &machineId, bool active)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Updating machine status: %1 to %2").arg(machineId).arg(active ? "active" : "inactive"));

    QJsonObject responseData;
    return m_apiManager->updateMachineStatus(machineId, active, responseData);
}

bool SessionManager::updateMachineLastSeen(const QString &machineId)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Updating machine last seen timestamp: %1").arg(machineId));

    QJsonObject responseData;
    return m_apiManager->updateMachineLastSeen(machineId, QDateTime::currentDateTime(), responseData);
}

bool SessionManager::checkServerConnection(bool &isConnected)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        isConnected = false;
        return false;
    }

    LOG_DEBUG("Checking server connection with ping");

    QJsonObject responseData;
    bool success = m_apiManager->ping(responseData);

    isConnected = success && responseData.contains("status") && responseData["status"].toString() == "ok";

    if (isConnected) {
        LOG_DEBUG("Server connection successful");
    } else {
        LOG_WARNING("Server connection failed");
    }

    return success;
}

bool SessionManager::processBatchData(const QUuid &sessionId, const QJsonArray &activityEvents, const QJsonArray &appUsages, const QJsonArray &systemMetrics, const QJsonArray &sessionEvents, QJsonObject &responseData)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("SessionManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Processing batch data for session: %1").arg(sessionId.toString()));

    QJsonObject batchData;
    batchData["session_id"] = sessionId.toString();

    if (!activityEvents.isEmpty()) {
        batchData["activity_events"] = activityEvents;
    }

    if (!appUsages.isEmpty()) {
        batchData["app_usages"] = appUsages;
    }

    if (!systemMetrics.isEmpty()) {
        batchData["system_metrics"] = systemMetrics;
    }

    if (!sessionEvents.isEmpty()) {
        batchData["session_events"] = sessionEvents;
    }

    return m_apiManager->processSessionBatch(sessionId, batchData, responseData);
}

void SessionManager::setMultiUserManager(MultiUserManager* userManager)
{
    if (userManager != m_multiUserManager) {
        LOG_INFO("Setting MultiUserManager in SessionManager");
        m_multiUserManager = userManager;
    }
}

MultiUserManager* SessionManager::getMultiUserManager() const
{
    // If we have a MultiUserManager set, return it
    if (m_multiUserManager) {
        return m_multiUserManager;
    }

    // If no MultiUserManager was explicitly set, log a warning
    LOG_WARNING("MultiUserManager requested but not set in SessionManager");
    return nullptr;
}
