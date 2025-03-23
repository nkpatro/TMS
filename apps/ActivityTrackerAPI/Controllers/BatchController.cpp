#include "BatchController.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QUrlQuery>
#include "logger/logger.h"
#include "httpserver/response.h"
#include "Core/ModelFactory.h"

BatchController::BatchController(QObject *parent)
    : ApiControllerBase(parent)
    , m_activityEventRepository(nullptr)
    , m_appUsageRepository(nullptr)
    , m_systemMetricsRepository(nullptr)
    , m_sessionEventRepository(nullptr)
    , m_sessionRepository(nullptr)
    , m_initialized(false)
{
    LOG_DEBUG("BatchController created");
}

BatchController::BatchController(
    ActivityEventRepository* activityRepo,
    AppUsageRepository* appUsageRepo,
    SystemMetricsRepository* metricsRepo,
    SessionEventRepository* sessionEventRepo,
    SessionRepository* sessionRepo,
    QObject *parent)
    : ApiControllerBase(parent)
    , m_activityEventRepository(activityRepo)
    , m_appUsageRepository(appUsageRepo)
    , m_systemMetricsRepository(metricsRepo)
    , m_sessionEventRepository(sessionEventRepo)
    , m_sessionRepository(sessionRepo)
    , m_initialized(false)
{
    LOG_DEBUG("BatchController created with existing repositories");

    // Check if all repositories are initialized
    if (m_activityEventRepository && m_activityEventRepository->isInitialized() &&
        m_appUsageRepository && m_appUsageRepository->isInitialized() &&
        m_systemMetricsRepository && m_systemMetricsRepository->isInitialized() &&
        m_sessionEventRepository && m_sessionEventRepository->isInitialized() &&
        m_sessionRepository && m_sessionRepository->isInitialized()) {
        m_initialized = true;
        LOG_INFO("BatchController initialized successfully");
    }
}

BatchController::~BatchController()
{
    // Only delete repositories if we created them (parent is this)
    if (m_activityEventRepository && m_activityEventRepository->parent() == this) {
        delete m_activityEventRepository;
        m_activityEventRepository = nullptr;
    }

    if (m_appUsageRepository && m_appUsageRepository->parent() == this) {
        delete m_appUsageRepository;
        m_appUsageRepository = nullptr;
    }

    if (m_systemMetricsRepository && m_systemMetricsRepository->parent() == this) {
        delete m_systemMetricsRepository;
        m_systemMetricsRepository = nullptr;
    }

    if (m_sessionEventRepository && m_sessionEventRepository->parent() == this) {
        delete m_sessionEventRepository;
        m_sessionEventRepository = nullptr;
    }

    if (m_sessionRepository && m_sessionRepository->parent() == this) {
        delete m_sessionRepository;
        m_sessionRepository = nullptr;
    }

    LOG_DEBUG("BatchController destroyed");
}

bool BatchController::initialize()
{
    if (m_initialized) {
        LOG_WARNING("BatchController already initialized");
        return true;
    }

    LOG_DEBUG("Initializing BatchController");

    try {
        // Check if repositories are valid and initialized
        if (!m_activityEventRepository) {
            LOG_ERROR("ActivityEvent repository not provided");
            return false;
        }

        if (!m_appUsageRepository) {
            LOG_ERROR("AppUsage repository not provided");
            return false;
        }

        if (!m_systemMetricsRepository) {
            LOG_ERROR("SystemMetrics repository not provided");
            return false;
        }

        if (!m_sessionEventRepository) {
            LOG_ERROR("SessionEvent repository not provided");
            return false;
        }

        if (!m_sessionRepository) {
            LOG_ERROR("Session repository not provided");
            return false;
        }

        if (!m_activityEventRepository->isInitialized()) {
            LOG_ERROR("ActivityEvent repository not initialized");
            return false;
        }

        if (!m_appUsageRepository->isInitialized()) {
            LOG_ERROR("AppUsage repository not initialized");
            return false;
        }

        if (!m_systemMetricsRepository->isInitialized()) {
            LOG_ERROR("SystemMetrics repository not initialized");
            return false;
        }

        if (!m_sessionEventRepository->isInitialized()) {
            LOG_ERROR("SessionEvent repository not initialized");
            return false;
        }

        if (!m_sessionRepository->isInitialized()) {
            LOG_ERROR("Session repository not initialized");
            return false;
        }

        m_initialized = true;
        LOG_INFO("BatchController initialized successfully");
        return true;
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception during BatchController initialization: %1").arg(e.what()));
        return false;
    }
}

void BatchController::setupRoutes(QHttpServer &server)
{
    if (!m_initialized) {
        LOG_ERROR("Cannot setup routes - BatchController not initialized");
        return;
    }

    LOG_INFO("Setting up BatchController routes");

    // Process batch data
    server.route("/api/batch", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleProcessBatch(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Process batch data for a specific session
    server.route("/api/sessions/<arg>/batch", QHttpServerRequest::Method::Post,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleProcessSessionBatch(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    LOG_INFO("BatchController routes configured");
}

QHttpServerResponse BatchController::handleProcessBatch(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("BatchController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing batch data request");

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized batch request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data in batch request");
            return createErrorResponse("Invalid JSON data", QHttpServerResponder::StatusCode::BadRequest);
        }

        // Validate required fields
        if (!json.contains("session_id") || json["session_id"].toString().isEmpty()) {
            LOG_WARNING("Missing required field: session_id");
            return createErrorResponse("Session ID is required", QHttpServerResponder::StatusCode::BadRequest);
        }

        QUuid sessionId = QUuid(json["session_id"].toString());
        QUuid userId = QUuid(userData["id"].toString());

        // Verify session exists
        auto session = m_sessionRepository->getById(sessionId);
        if (!session) {
            LOG_WARNING(QString("Session not found with ID: %1").arg(sessionId.toString()));
            return Http::Response::notFound("Session not found");
        }

        // Initialize results object
        QJsonObject results;
        results["session_id"] = sessionId.toString(QUuid::WithoutBraces);
        results["processing_time"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        results["success"] = true;
        results["processed_counts"] = QJsonObject();

        // Process different types of batch data if available
        bool hasAnyData = false;

        // Process activity events
        if (json.contains("activity_events") && json["activity_events"].isArray()) {
            QJsonArray events = json["activity_events"].toArray();
            hasAnyData = true;
            if (!processActivityEvents(events, sessionId, userId, results)) {
                results["success"] = false;
            }
        }

        // Process app usages
        if (json.contains("app_usages") && json["app_usages"].isArray()) {
            QJsonArray appUsages = json["app_usages"].toArray();
            hasAnyData = true;
            if (!processAppUsages(appUsages, sessionId, userId, results)) {
                results["success"] = false;
            }
        }

        // Process system metrics
        if (json.contains("system_metrics") && json["system_metrics"].isArray()) {
            QJsonArray metrics = json["system_metrics"].toArray();
            hasAnyData = true;
            if (!processSystemMetrics(metrics, sessionId, userId, results)) {
                results["success"] = false;
            }
        }

        // Process session events
        if (json.contains("session_events") && json["session_events"].isArray()) {
            QJsonArray events = json["session_events"].toArray();
            hasAnyData = true;
            if (!processSessionEvents(events, sessionId, userId, results)) {
                results["success"] = false;
            }
        }

        if (!hasAnyData) {
            LOG_WARNING("Batch request contained no valid data arrays");
            return createErrorResponse("No valid data arrays found in request", QHttpServerResponder::StatusCode::BadRequest);
        }

        LOG_INFO(QString("Batch processing completed for session %1").arg(sessionId.toString()));
        return createSuccessResponse(results);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception processing batch: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to process batch: %1").arg(e.what()));
    }
}

QHttpServerResponse BatchController::handleProcessSessionBatch(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("BatchController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing batch data for session ID: %1").arg(sessionId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized batch request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Verify session exists
        auto session = m_sessionRepository->getById(sessionUuid);
        if (!session) {
            LOG_WARNING(QString("Session not found with ID: %1").arg(sessionId));
            return Http::Response::notFound("Session not found");
        }

        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data in batch request");
            return createErrorResponse("Invalid JSON data", QHttpServerResponder::StatusCode::BadRequest);
        }

        QUuid userId = QUuid(userData["id"].toString());

        // Initialize results object
        QJsonObject results;
        results["session_id"] = sessionUuid.toString(QUuid::WithoutBraces);
        results["processing_time"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        results["success"] = true;
        results["processed_counts"] = QJsonObject();

        // Process different types of batch data if available
        bool hasAnyData = false;

        // Process activity events
        if (json.contains("activity_events") && json["activity_events"].isArray()) {
            QJsonArray events = json["activity_events"].toArray();
            hasAnyData = true;
            if (!processActivityEvents(events, sessionUuid, userId, results)) {
                results["success"] = false;
            }
        }

        // Process app usages
        if (json.contains("app_usages") && json["app_usages"].isArray()) {
            QJsonArray appUsages = json["app_usages"].toArray();
            hasAnyData = true;
            if (!processAppUsages(appUsages, sessionUuid, userId, results)) {
                results["success"] = false;
            }
        }

        // Process system metrics
        if (json.contains("system_metrics") && json["system_metrics"].isArray()) {
            QJsonArray metrics = json["system_metrics"].toArray();
            hasAnyData = true;
            if (!processSystemMetrics(metrics, sessionUuid, userId, results)) {
                results["success"] = false;
            }
        }

        // Process session events
        if (json.contains("session_events") && json["session_events"].isArray()) {
            QJsonArray events = json["session_events"].toArray();
            hasAnyData = true;
            if (!processSessionEvents(events, sessionUuid, userId, results)) {
                results["success"] = false;
            }
        }

        if (!hasAnyData) {
            LOG_WARNING("Batch request contained no valid data arrays");
            return createErrorResponse("No valid data arrays found in request", QHttpServerResponder::StatusCode::BadRequest);
        }

        LOG_INFO(QString("Batch processing completed for session %1").arg(sessionId));
        return createSuccessResponse(results);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception processing batch for session: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to process batch: %1").arg(e.what()));
    }
}

bool BatchController::processActivityEvents(const QJsonArray &events, QUuid sessionId, QUuid userId, QJsonObject &results)
{
    LOG_DEBUG(QString("Processing %1 activity events").arg(events.size()));

    int successCount = 0;
    int failureCount = 0;
    QJsonArray failures;

    for (int i = 0; i < events.size(); i++) {
        if (!events[i].isObject()) {
            LOG_WARNING(QString("Invalid activity event at index %1 - not an object").arg(i));
            failureCount++;
            failures.append(QJsonObject{{"index", i}, {"error", "Not a valid JSON object"}});
            continue;
        }

        QJsonObject eventData = events[i].toObject();

        try {
            // Create activity event
            ActivityEventModel *event = new ActivityEventModel();
            event->setSessionId(sessionId);

            // Set event type
            if (eventData.contains("event_type") && !eventData["event_type"].toString().isEmpty()) {
                // Convert string event type to enum
                QString eventTypeStr = eventData["event_type"].toString();
                EventTypes::ActivityEventType eventType;

                if (eventTypeStr == "mouse_click") {
                    eventType = EventTypes::ActivityEventType::MouseClick;
                } else if (eventTypeStr == "mouse_move") {
                    eventType = EventTypes::ActivityEventType::MouseMove;
                } else if (eventTypeStr == "keyboard") {
                    eventType = EventTypes::ActivityEventType::Keyboard;
                } else if (eventTypeStr == "afk_start") {
                    eventType = EventTypes::ActivityEventType::AfkStart;
                } else if (eventTypeStr == "afk_end") {
                    eventType = EventTypes::ActivityEventType::AfkEnd;
                } else if (eventTypeStr == "app_focus") {
                    eventType = EventTypes::ActivityEventType::AppFocus;
                } else if (eventTypeStr == "app_unfocus") {
                    eventType = EventTypes::ActivityEventType::AppUnfocus;
                } else {
                    // Default to mouse click if unknown
                    eventType = EventTypes::ActivityEventType::MouseClick;
                }

                event->setEventType(eventType);
            } else {
                event->setEventType(EventTypes::ActivityEventType::MouseClick);
            }

            // Set app ID if provided
            if (eventData.contains("app_id") && !eventData["app_id"].toString().isEmpty()) {
                event->setAppId(QUuid(eventData["app_id"].toString()));
            }

            // Set event time
            if (eventData.contains("event_time") && !eventData["event_time"].toString().isEmpty()) {
                QDateTime eventTime = QDateTime::fromString(eventData["event_time"].toString(), Qt::ISODate);
                if (eventTime.isValid()) {
                    event->setEventTime(eventTime);
                } else {
                    event->setEventTime(QDateTime::currentDateTimeUtc());
                }
            } else {
                event->setEventTime(QDateTime::currentDateTimeUtc());
            }

            // Set event data
            if (eventData.contains("event_data") && eventData["event_data"].isObject()) {
                event->setEventData(eventData["event_data"].toObject());
            }

            // Set metadata
            event->setCreatedBy(userId);
            event->setUpdatedBy(userId);
            event->setCreatedAt(QDateTime::currentDateTimeUtc());
            event->setUpdatedAt(QDateTime::currentDateTimeUtc());

            // Save event
            bool success = m_activityEventRepository->save(event);

            if (success) {
                successCount++;
            } else {
                failureCount++;
                failures.append(QJsonObject{{"index", i}, {"error", "Failed to save to database"}});
            }

            delete event;
        }
        catch (const std::exception &e) {
            LOG_ERROR(QString("Exception processing session event at index %1: %2").arg(i).arg(e.what()));
            failureCount++;
            failures.append(QJsonObject{
                {"index", i},
                {"error", QString("Exception: %1").arg(e.what())}
            });
        }
    }

    // Update results
    QJsonObject counts = results["processed_counts"].toObject();
    counts["session_events_success"] = successCount;
    counts["session_events_failure"] = failureCount;
    counts["session_events_total"] = events.size();
    results["processed_counts"] = counts;

    if (failureCount > 0) {
        results["session_events_failures"] = failures;
    }

    LOG_INFO(QString("Processed %1 session events: %2 successful, %3 failed")
            .arg(events.size()).arg(successCount).arg(failureCount));

    return (failureCount == 0);
}

QJsonObject BatchController::extractJsonFromRequest(const QHttpServerRequest &request, bool &ok)
{
    ok = false;

    // Parse JSON body
    QByteArray body = request.body();
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (doc.isNull() || !doc.isObject()) {
        LOG_WARNING("Failed to parse JSON from request body");
        return QJsonObject();
    }

    ok = true;
    LOG_DEBUG(QString("Extracted JSON: %1").arg(QString::fromUtf8(body)));
    return doc.object();
}

QUuid BatchController::stringToUuid(const QString &str) const
{
    // Handle both simple format and UUID format
    if (str.contains('-')) {
        return QUuid(str);
    } else {
        // Create UUID from simple format (without dashes)
        QString withDashes = str;
        withDashes.insert(8, '-');
        withDashes.insert(13, '-');
        withDashes.insert(18, '-');
        withDashes.insert(23, '-');
        return QUuid(withDashes);
    }
}

QString BatchController::uuidToString(const QUuid &uuid) const
{
    return uuid.toString(QUuid::WithoutBraces);
}

bool BatchController::processAppUsages(const QJsonArray &appUsages, QUuid sessionId, QUuid userId, QJsonObject &results)
{
    LOG_DEBUG(QString("Processing %1 app usages").arg(appUsages.size()));

    int successCount = 0;
    int failureCount = 0;
    QJsonArray failures;

    for (int i = 0; i < appUsages.size(); i++) {
        if (!appUsages[i].isObject()) {
            LOG_WARNING(QString("Invalid app usage at index %1 - not an object").arg(i));
            failureCount++;
            failures.append(QJsonObject{{"index", i}, {"error", "Not a valid JSON object"}});
            continue;
        }

        QJsonObject usageData = appUsages[i].toObject();

        try {
            // Create app usage
            AppUsageModel *appUsage = new AppUsageModel();
            appUsage->setSessionId(sessionId);

            // Check required app_id
            if (!usageData.contains("app_id") || usageData["app_id"].toString().isEmpty()) {
                LOG_WARNING(QString("App usage at index %1 missing required app_id").arg(i));
                failureCount++;
                failures.append(QJsonObject{{"index", i}, {"error", "Missing required app_id"}});
                delete appUsage;
                continue;
            }

            appUsage->setAppId(QUuid(usageData["app_id"].toString()));

            // Set window title if provided
            if (usageData.contains("window_title")) {
                appUsage->setWindowTitle(usageData["window_title"].toString());
            }

            // Set start time
            if (usageData.contains("start_time") && !usageData["start_time"].toString().isEmpty()) {
                QDateTime startTime = QDateTime::fromString(usageData["start_time"].toString(), Qt::ISODate);
                if (startTime.isValid()) {
                    appUsage->setStartTime(startTime);
                } else {
                    appUsage->setStartTime(QDateTime::currentDateTimeUtc());
                }
            } else {
                appUsage->setStartTime(QDateTime::currentDateTimeUtc());
            }

            // Set end time if provided (for completed app usages)
            if (usageData.contains("end_time") && !usageData["end_time"].toString().isEmpty()) {
                QDateTime endTime = QDateTime::fromString(usageData["end_time"].toString(), Qt::ISODate);
                if (endTime.isValid()) {
                    appUsage->setEndTime(endTime);
                }
            }

            // Set metadata
            appUsage->setCreatedBy(userId);
            appUsage->setUpdatedBy(userId);
            appUsage->setCreatedAt(QDateTime::currentDateTimeUtc());
            appUsage->setUpdatedAt(QDateTime::currentDateTimeUtc());

            // Save app usage
            bool success = m_appUsageRepository->save(appUsage);

            if (success) {
                successCount++;
            } else {
                failureCount++;
                failures.append(QJsonObject{{"index", i}, {"error", "Failed to save to database"}});
            }

            delete appUsage;
        }
        catch (const std::exception &e) {
            LOG_ERROR(QString("Exception processing app usage at index %1: %2").arg(i).arg(e.what()));
            failureCount++;
            failures.append(QJsonObject{
                {"index", i},
                {"error", QString("Exception: %1").arg(e.what())}
            });
        }
    }

    // Update results
    QJsonObject counts = results["processed_counts"].toObject();
    counts["app_usages_success"] = successCount;
    counts["app_usages_failure"] = failureCount;
    counts["app_usages_total"] = appUsages.size();
    results["processed_counts"] = counts;

    if (failureCount > 0) {
        results["app_usages_failures"] = failures;
    }

    LOG_INFO(QString("Processed %1 app usages: %2 successful, %3 failed")
            .arg(appUsages.size()).arg(successCount).arg(failureCount));

    return (failureCount == 0);
}

bool BatchController::processSystemMetrics(const QJsonArray &metrics, QUuid sessionId, QUuid userId, QJsonObject &results)
{
    LOG_DEBUG(QString("Processing %1 system metrics").arg(metrics.size()));

    int successCount = 0;
    int failureCount = 0;
    QJsonArray failures;

    for (int i = 0; i < metrics.size(); i++) {
        if (!metrics[i].isObject()) {
            LOG_WARNING(QString("Invalid system metric at index %1 - not an object").arg(i));
            failureCount++;
            failures.append(QJsonObject{{"index", i}, {"error", "Not a valid JSON object"}});
            continue;
        }

        QJsonObject metricData = metrics[i].toObject();

        try {
            // Create system metrics
            SystemMetricsModel *systemMetric = new SystemMetricsModel();
            systemMetric->setSessionId(sessionId);

            // Set CPU usage
            if (metricData.contains("cpu_usage") && metricData["cpu_usage"].isDouble()) {
                systemMetric->setCpuUsage(metricData["cpu_usage"].toDouble());
            } else {
                systemMetric->setCpuUsage(0.0);
            }

            // Set GPU usage
            if (metricData.contains("gpu_usage") && metricData["gpu_usage"].isDouble()) {
                systemMetric->setGpuUsage(metricData["gpu_usage"].toDouble());
            } else {
                systemMetric->setGpuUsage(0.0);
            }

            // Set memory usage
            if (metricData.contains("memory_usage") && metricData["memory_usage"].isDouble()) {
                systemMetric->setMemoryUsage(metricData["memory_usage"].toDouble());
            } else {
                systemMetric->setMemoryUsage(0.0);
            }

            // Set measurement time
            if (metricData.contains("measurement_time") && !metricData["measurement_time"].toString().isEmpty()) {
                QDateTime measurementTime = QDateTime::fromString(metricData["measurement_time"].toString(), Qt::ISODate);
                if (measurementTime.isValid()) {
                    systemMetric->setMeasurementTime(measurementTime);
                } else {
                    systemMetric->setMeasurementTime(QDateTime::currentDateTimeUtc());
                }
            } else {
                systemMetric->setMeasurementTime(QDateTime::currentDateTimeUtc());
            }

            // Set metadata
            systemMetric->setCreatedBy(userId);
            systemMetric->setUpdatedBy(userId);
            systemMetric->setCreatedAt(QDateTime::currentDateTimeUtc());
            systemMetric->setUpdatedAt(QDateTime::currentDateTimeUtc());

            // Save system metric
            bool success = m_systemMetricsRepository->save(systemMetric);

            if (success) {
                successCount++;
            } else {
                failureCount++;
                failures.append(QJsonObject{{"index", i}, {"error", "Failed to save to database"}});
            }

            delete systemMetric;
        }
        catch (const std::exception &e) {
            LOG_ERROR(QString("Exception processing system metric at index %1: %2").arg(i).arg(e.what()));
            failureCount++;
            failures.append(QJsonObject{
                {"index", i},
                {"error", QString("Exception: %1").arg(e.what())}
            });
        }
    }

    // Update results
    QJsonObject counts = results["processed_counts"].toObject();
    counts["system_metrics_success"] = successCount;
    counts["system_metrics_failure"] = failureCount;
    counts["system_metrics_total"] = metrics.size();
    results["processed_counts"] = counts;

    if (failureCount > 0) {
        results["system_metrics_failures"] = failures;
    }

    LOG_INFO(QString("Processed %1 system metrics: %2 successful, %3 failed")
            .arg(metrics.size()).arg(successCount).arg(failureCount));

    return (failureCount == 0);
}

bool BatchController::processSessionEvents(const QJsonArray &events, QUuid sessionId, QUuid userId, QJsonObject &results)
{
    LOG_DEBUG(QString("Processing %1 session events").arg(events.size()));

    int successCount = 0;
    int failureCount = 0;
    QJsonArray failures;

    for (int i = 0; i < events.size(); i++) {
        if (!events[i].isObject()) {
            LOG_WARNING(QString("Invalid session event at index %1 - not an object").arg(i));
            failureCount++;
            failures.append(QJsonObject{{"index", i}, {"error", "Not a valid JSON object"}});
            continue;
        }

        QJsonObject eventData = events[i].toObject();

        try {
            // Create session event - note the correct type: SessionEventModel, not ActivityEventModel
            SessionEventModel *event = new SessionEventModel();
            event->setSessionId(sessionId);

            // Set event type
            if (eventData.contains("event_type") && !eventData["event_type"].toString().isEmpty()) {
                QString eventTypeStr = eventData["event_type"].toString();
                EventTypes::SessionEventType eventType;

                if (eventTypeStr == "login") {
                    eventType = EventTypes::SessionEventType::Login;
                } else if (eventTypeStr == "logout") {
                    eventType = EventTypes::SessionEventType::Logout;
                } else if (eventTypeStr == "lock") {
                    eventType = EventTypes::SessionEventType::Lock;
                } else if (eventTypeStr == "unlock") {
                    eventType = EventTypes::SessionEventType::Unlock;
                } else if (eventTypeStr == "switch_user") {
                    eventType = EventTypes::SessionEventType::SwitchUser;
                } else if (eventTypeStr == "remote_connect") {
                    eventType = EventTypes::SessionEventType::RemoteConnect;
                } else if (eventTypeStr == "remote_disconnect") {
                    eventType = EventTypes::SessionEventType::RemoteDisconnect;
                } else {
                    // Default to login if unknown
                    eventType = EventTypes::SessionEventType::Login;
                    LOG_WARNING(QString("Unknown session event type: %1, defaulting to Login").arg(eventTypeStr));
                }

                event->setEventType(eventType);
            } else {
                event->setEventType(EventTypes::SessionEventType::Login);
                LOG_WARNING("Session event missing event_type, defaulting to Login");
            }

            // Set user ID if provided
            if (eventData.contains("user_id") && !eventData["user_id"].toString().isEmpty()) {
                event->setUserId(QUuid(eventData["user_id"].toString()));
            } else {
                // Default to the authenticated user
                event->setUserId(userId);
            }

            // Set previous user ID if provided (for switch user events)
            if (eventData.contains("previous_user_id") && !eventData["previous_user_id"].toString().isEmpty()) {
                event->setPreviousUserId(QUuid(eventData["previous_user_id"].toString()));
            }

            // Set machine ID if provided
            if (eventData.contains("machine_id") && !eventData["machine_id"].toString().isEmpty()) {
                event->setMachineId(QUuid(eventData["machine_id"].toString()));
            } else {
                LOG_WARNING(QString("Session event at index %1 missing machine_id").arg(i));
                // We'll still process it, but machine ID is generally expected
            }

            // Set terminal session ID if provided
            if (eventData.contains("terminal_session_id") && !eventData["terminal_session_id"].toString().isEmpty()) {
                event->setTerminalSessionId(eventData["terminal_session_id"].toString());
            }

            // Set is_remote flag if provided
            if (eventData.contains("is_remote")) {
                event->setIsRemote(eventData["is_remote"].toBool());
            }

            // Set event time
            if (eventData.contains("event_time") && !eventData["event_time"].toString().isEmpty()) {
                QDateTime eventTime = QDateTime::fromString(eventData["event_time"].toString(), Qt::ISODate);
                if (eventTime.isValid()) {
                    event->setEventTime(eventTime);
                } else {
                    LOG_WARNING(QString("Invalid event_time format at index %1, using current time").arg(i));
                    event->setEventTime(QDateTime::currentDateTimeUtc());
                }
            } else {
                event->setEventTime(QDateTime::currentDateTimeUtc());
            }

            // Set event data
            if (eventData.contains("event_data") && eventData["event_data"].isObject()) {
                event->setEventData(eventData["event_data"].toObject());
            }

            // Set metadata
            event->setCreatedBy(userId);
            event->setUpdatedBy(userId);
            event->setCreatedAt(QDateTime::currentDateTimeUtc());
            event->setUpdatedAt(QDateTime::currentDateTimeUtc());

            // Save event using the SessionEventRepository (fixed)
            bool success = m_sessionEventRepository->save(event);

            if (success) {
                successCount++;
                LOG_DEBUG(QString("Successfully saved session event %1 of type %2")
                         .arg(event->id().toString(), event->eventType() == EventTypes::SessionEventType::Login ? "Login" :
                             (event->eventType() == EventTypes::SessionEventType::Logout ? "Logout" : "Other")));
            } else {
                failureCount++;
                LOG_ERROR(QString("Failed to save session event at index %1").arg(i));
                failures.append(QJsonObject{{"index", i}, {"error", "Failed to save to database"}});
            }

            delete event;
        }
        catch (const std::exception &e) {
            LOG_ERROR(QString("Exception processing session event at index %1: %2").arg(i).arg(e.what()));
            failureCount++;
            failures.append(QJsonObject{
                {"index", i},
                {"error", QString("Exception: %1").arg(e.what())}
            });
        }
    }

    // Update results
    QJsonObject counts = results["processed_counts"].toObject();
    counts["session_events_success"] = successCount;
    counts["session_events_failure"] = failureCount;
    counts["session_events_total"] = events.size();
    results["processed_counts"] = counts;

    if (failureCount > 0) {
        results["session_events_failures"] = failures;
    }

    LOG_INFO(QString("Processed %1 session events: %2 successful, %3 failed")
            .arg(events.size()).arg(successCount).arg(failureCount));

    return (failureCount == 0);
}