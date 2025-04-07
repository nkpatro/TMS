#include "AppUsageController.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QUrlQuery>
#include <Core/ModelFactory.h>

#include "logger/logger.h"
#include "httpserver/response.h"

// Constructor with default parameters
AppUsageController::AppUsageController(QObject *parent)
    : ApiControllerBase(parent)  // Changed from Http::Controller to ApiControllerBase
    , m_appUsageRepository(nullptr)
    , m_applicationRepository(nullptr)
    , m_initialized(false)
{
    LOG_DEBUG("AppUsageController created");
}

// Constructor with repositories
AppUsageController::AppUsageController(AppUsageRepository* appUsageRepository,
                                     ApplicationRepository* applicationRepository,
                                     QObject *parent)
    : ApiControllerBase(parent)  // Changed from Http::Controller to ApiControllerBase
    , m_appUsageRepository(appUsageRepository)
    , m_applicationRepository(applicationRepository)
    , m_initialized(false)
{
    LOG_DEBUG("AppUsageController created with existing repositories");

    // If repositories were provided, check if they're initialized
    if (m_appUsageRepository && m_applicationRepository &&
        m_appUsageRepository->isInitialized() &&
        m_applicationRepository->isInitialized()) {
        m_initialized = true;
        LOG_INFO("AppUsageController initialized successfully");
        }
}

// Destructor
AppUsageController::~AppUsageController()
{
    // Only delete repositories if we created them
    if (m_appUsageRepository && m_appUsageRepository->parent() == this) {
        delete m_appUsageRepository;
        m_appUsageRepository = nullptr;
    }

    if (m_applicationRepository && m_applicationRepository->parent() == this) {
        delete m_applicationRepository;
        m_applicationRepository = nullptr;
    }

    LOG_DEBUG("AppUsageController destroyed");
}

bool AppUsageController::initialize()
{
    if (m_initialized) {
        LOG_WARNING("AppUsageController already initialized");
        return true;
    }

    LOG_DEBUG("Initializing AppUsageController");

    try {
        // Check if repositories are valid and initialized
        if (!m_appUsageRepository) {
            LOG_ERROR("AppUsage repository not provided");
            return false;
        }

        if (!m_applicationRepository) {
            LOG_ERROR("Application repository not provided");
            return false;
        }

        if (!m_appUsageRepository->isInitialized()) {
            LOG_ERROR("AppUsage repository not initialized");
            return false;
        }

        if (!m_applicationRepository->isInitialized()) {
            LOG_ERROR("Application repository not initialized");
            return false;
        }

        m_initialized = true;
        LOG_INFO("AppUsageController initialized successfully");
        return true;
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception during AppUsageController initialization: %1").arg(e.what()));
        return false;
    }
}

void AppUsageController::setupRoutes(QHttpServer &server)
{
    LOG_INFO("Setting up AppUsageController routes");

    // Get all app usages
    server.route("/api/app-usages", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetAppUsages(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get app usage by ID
    server.route("/api/app-usages/<arg>", QHttpServerRequest::Method::Get,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetAppUsageById(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get app usages by session ID
    server.route("/api/sessions/<arg>/app-usages", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetAppUsagesBySessionId(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get app usages by app ID
    server.route("/api/applications/<arg>/usages", QHttpServerRequest::Method::Get,
        [this](const qint64 appId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetAppUsagesByAppId(appId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Start app usage
    server.route("/api/app-usages", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleStartAppUsage(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // End app usage
    server.route("/api/app-usages/<arg>/end", QHttpServerRequest::Method::Post,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleEndAppUsage(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Update app usage
    server.route("/api/app-usages/<arg>", QHttpServerRequest::Method::Put,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleUpdateAppUsage(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get app usage stats for a session
    server.route("/api/sessions/<arg>/app-usages/stats", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetSessionAppUsageStats(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get top apps for a session
    server.route("/api/sessions/<arg>/app-usages/top", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetTopApps(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get active apps for a session
    server.route("/api/sessions/<arg>/app-usages/active", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetActiveApps(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    LOG_INFO("AppUsageController routes configured");
}

QHttpServerResponse AppUsageController::handleGetAppUsages(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("AppUsageController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET all app usages request");

    // Check authentication - using base class method instead of custom isAuthorized
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        // Parse query parameters
        QUrlQuery query(request.url().query());
        int limit = query.queryItemValue("limit", QUrl::FullyDecoded).toInt();
        if (limit <= 0 || limit > 1000) {
            limit = 100; // Default limit
        }

        // Get all app usages (limited)
        QList<QSharedPointer<AppUsageModel>> appUsages = m_appUsageRepository->getAll();

        // Limit the results
        if (appUsages.size() > limit) {
            appUsages = appUsages.mid(0, limit);
        }

        QJsonArray appUsagesArray;
        for (const auto &usage : appUsages) {
            appUsagesArray.append(appUsageToJson(usage.data()));
        }

        return createSuccessResponse(appUsagesArray);  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting app usages: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve app usages: %1").arg(e.what()));  // Using base class method
    }
}

QHttpServerResponse AppUsageController::handleGetAppUsageById(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("AppUsageController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET app usage by ID request: %1").arg(id));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid usageId = stringToUuid(QString::number(id));
        auto appUsage = m_appUsageRepository->getById(usageId);

        if (!appUsage) {
            LOG_WARNING(QString("App usage not found with ID: %1").arg(id));
            return Http::Response::notFound("App usage not found");
        }

        return createSuccessResponse(appUsageToJson(appUsage.data()));  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting app usage: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve app usage: %1").arg(e.what()));  // Using base class method
    }
}

QHttpServerResponse AppUsageController::handleGetAppUsagesBySessionId(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("AppUsageController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET app usages by session ID request: %1").arg(sessionId));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Parse query parameters
        QUrlQuery query(request.url().query());
        bool activeOnly = (query.queryItemValue("active", QUrl::FullyDecoded) == "true");

        QList<QSharedPointer<AppUsageModel>> appUsages;

        if (activeOnly) {
            appUsages = m_appUsageRepository->getActiveAppUsages(sessionUuid);
            LOG_INFO(QString("Retrieved %1 active app usages for session %2").arg(appUsages.size()).arg(sessionId));
        } else {
            appUsages = m_appUsageRepository->getBySessionId(sessionUuid);
            LOG_INFO(QString("Retrieved %1 app usages for session %2").arg(appUsages.size()).arg(sessionId));
        }

        QJsonArray appUsagesArray;
        for (const auto &usage : appUsages) {
            appUsagesArray.append(appUsageToJson(usage.data()));
        }

        return createSuccessResponse(appUsagesArray);  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting app usages by session ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve app usages: %1").arg(e.what()));  // Using base class method
    }
}

QHttpServerResponse AppUsageController::handleGetAppUsagesByAppId(const qint64 appId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("AppUsageController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET app usages by app ID request: %1").arg(appId));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid appUuid = stringToUuid(QString::number(appId));

        // Verify application exists
        auto application = m_applicationRepository->getById(appUuid);
        if (!application) {
            LOG_WARNING(QString("Application not found with ID: %1").arg(appId));
            return Http::Response::notFound("Application not found");
        }

        // Get app usages for the application
        QList<QSharedPointer<AppUsageModel>> appUsages = m_appUsageRepository->getByAppId(appUuid);

        // Parse query parameters for limiting results manually
        QUrlQuery query(request.url().query());
        int limit = query.queryItemValue("limit", QUrl::FullyDecoded).toInt();
        if (limit > 0 && limit < appUsages.size()) {
            appUsages = appUsages.mid(0, limit);
        }

        QJsonArray appUsagesArray;
        for (const auto &usage : appUsages) {
            appUsagesArray.append(appUsageToJson(usage.data()));
        }

        LOG_INFO(QString("Retrieved %1 app usages for app %2").arg(appUsages.size()).arg(appId));
        return createSuccessResponse(appUsagesArray);  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting app usages by app ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve app usages: %1").arg(e.what()));  // Using base class method
    }
}

QHttpServerResponse AppUsageController::handleStartAppUsage(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("AppUsageController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing START app usage request");

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data");
            return Http::Response::badRequest("Invalid JSON data");
        }

        // Check required fields
        if (!json.contains("session_id") || json["session_id"].toString().isEmpty()) {
            LOG_WARNING("Session ID is required");
            return Http::Response::badRequest("Session ID is required");
        }

        if (!json.contains("app_id") || json["app_id"].toString().isEmpty()) {
            LOG_WARNING("App ID is required");
            return Http::Response::badRequest("App ID is required");
        }

        QUuid sessionId = QUuid(json["session_id"].toString());
        QUuid appId = QUuid(json["app_id"].toString());

        // Verify application exists
        auto application = m_applicationRepository->getById(appId);
        if (!application) {
            LOG_WARNING(QString("Application not found with ID: %1").arg(json["app_id"].toString()));
            return Http::Response::notFound("Application not found");
        }

        // Create new app usage
        AppUsageModel *appUsage = new AppUsageModel();
        appUsage->setSessionId(sessionId);
        appUsage->setAppId(appId);

        // Set window title if provided
        if (json.contains("window_title") && !json["window_title"].toString().isEmpty()) {
            appUsage->setWindowTitle(json["window_title"].toString());
        }

        // Set start time
        if (json.contains("start_time") && !json["start_time"].toString().isEmpty()) {
            appUsage->setStartTime(QDateTime::fromString(json["start_time"].toString(), Qt::ISODate));
        } else {
            appUsage->setStartTime(QDateTime::currentDateTimeUtc());
        }

        // Set metadata using ModelFactory
        ModelFactory::setCreationTimestamps(appUsage, QUuid(userData["id"].toString()));

        bool success = m_appUsageRepository->save(appUsage);

        if (!success) {
            delete appUsage;
            LOG_ERROR("Failed to start app usage");
            return createErrorResponse("Failed to start app usage", QHttpServerResponder::StatusCode::InternalServerError);
        }

        QJsonObject response = appUsageToJson(appUsage);
        LOG_INFO(QString("App usage started successfully: %1 for app %2")
                .arg(appUsage->id().toString(), application->appName()));
        delete appUsage;

        return createSuccessResponse(response, QHttpServerResponder::StatusCode::Created);  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception starting app usage: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to start app usage: %1").arg(e.what()));  // Using base class method
    }
}

QHttpServerResponse AppUsageController::handleEndAppUsage(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("AppUsageController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing END app usage request: %1").arg(id));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid usageId = stringToUuid(QString::number(id));

        // Get the app usage
        auto appUsage = m_appUsageRepository->getById(usageId);
        if (!appUsage) {
            LOG_WARNING(QString("App usage not found with ID: %1").arg(id));
            return Http::Response::notFound("App usage not found");
        }

        // Check if already ended
        if (!appUsage->isActive()) {
            LOG_WARNING(QString("App usage %1 is already ended").arg(id));
            return Http::Response::badRequest("App usage is already ended");
        }

        // Parse request for end time
        QDateTime endTime;
        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);

        if (ok && json.contains("end_time") && !json["end_time"].toString().isEmpty()) {
            endTime = QDateTime::fromString(json["end_time"].toString(), Qt::ISODate);
        } else {
            endTime = QDateTime::currentDateTimeUtc();
        }

        // Update app usage metadata using ModelFactory
        ModelFactory::setUpdateTimestamps(appUsage.data(), QUuid(userData["id"].toString()));

        // End the app usage
        bool success = m_appUsageRepository->endAppUsage(usageId, endTime);

        if (!success) {
            LOG_ERROR(QString("Failed to end app usage: %1").arg(id));
            return createErrorResponse("Failed to end app usage", QHttpServerResponder::StatusCode::InternalServerError);
        }

        // Reload the app usage to get updated data
        appUsage = m_appUsageRepository->getById(usageId);

        // Get the application name for logging
        auto application = m_applicationRepository->getById(appUsage->appId());
        QString appName = application ? application->appName() : appUsage->appId().toString();

        LOG_INFO(QString("App usage ended successfully: %1 for app %2, duration: %3 seconds")
                .arg(appUsage->id().toString(), appName)
                .arg(appUsage->duration()));

        return createSuccessResponse(appUsageToJson(appUsage.data()));  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception ending app usage: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to end app usage: %1").arg(e.what()));  // Using base class method
    }
}

QHttpServerResponse AppUsageController::handleUpdateAppUsage(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("AppUsageController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing UPDATE app usage request: %1").arg(id));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid usageId = stringToUuid(QString::number(id));

        // Get the app usage
        auto appUsage = m_appUsageRepository->getById(usageId);
        if (!appUsage) {
            LOG_WARNING(QString("App usage not found with ID: %1").arg(id));
            return Http::Response::notFound("App usage not found");
        }

        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data");
            return Http::Response::badRequest("Invalid JSON data");
        }

        // Update window title if provided
        if (json.contains("window_title")) {
            appUsage->setWindowTitle(json["window_title"].toString());
        }

        // Update start time if provided and app usage is active
        if (json.contains("start_time") && appUsage->isActive()) {
            appUsage->setStartTime(QDateTime::fromString(json["start_time"].toString(), Qt::ISODate));
        }

        // Update end time if provided and app usage is not active
        if (json.contains("end_time") && !appUsage->isActive()) {
            appUsage->setEndTime(QDateTime::fromString(json["end_time"].toString(), Qt::ISODate));
        }

        // Update metadata using ModelFactory
        ModelFactory::setUpdateTimestamps(appUsage.data(), QUuid(userData["id"].toString()));

        // Save updates
        bool success = m_appUsageRepository->update(appUsage.data());

        if (!success) {
            LOG_ERROR(QString("Failed to update app usage: %1").arg(id));
            return createErrorResponse("Failed to update app usage", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("App usage updated successfully: %1").arg(id));
        return createSuccessResponse(appUsageToJson(appUsage.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception updating app usage: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to update app usage: %1").arg(e.what()));
    }
}

QHttpServerResponse AppUsageController::handleGetSessionAppUsageStats(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("AppUsageController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session app usage stats request: %1").arg(sessionId));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Get app usage summary for the session
        QJsonObject summary = m_appUsageRepository->getAppUsageSummary(sessionUuid);

        if (summary.isEmpty()) {
            LOG_WARNING(QString("No app usage data found for session: %1").arg(sessionId));
            return createSuccessResponse(QJsonObject{{"message", "No app usage data found for this session"}});
        }

        // Add session ID to the response
        summary["session_id"] = uuidToString(sessionUuid);

        LOG_INFO(QString("App usage stats retrieved for session: %1").arg(sessionId));
        return createSuccessResponse(summary);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting app usage stats: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve app usage stats: %1").arg(e.what()));
    }
}

QHttpServerResponse AppUsageController::handleGetTopApps(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("AppUsageController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET top apps request for session: %1").arg(sessionId));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Parse query parameters
        QUrlQuery query(request.url().query());
        int limit = query.queryItemValue("limit", QUrl::FullyDecoded).toInt();

        if (limit <= 0 || limit > 100) {
            limit = 10; // Default limit
        }

        // Get top apps for the session
        QJsonArray topApps = m_appUsageRepository->getTopApps(sessionUuid, limit);

        if (topApps.isEmpty()) {
            LOG_WARNING(QString("No app usage data found for session: %1").arg(sessionId));
            return createSuccessResponse(QJsonObject{
                {"message", "No app usage data found for this session"},
                {"session_id", uuidToString(sessionUuid)}
            });
        }

        // Create response with the top apps and metadata
        QJsonObject response;
        response["session_id"] = uuidToString(sessionUuid);
        response["limit"] = limit;
        response["top_apps"] = topApps;

        LOG_INFO(QString("Top %1 apps retrieved for session: %2").arg(limit).arg(sessionId));
        return createSuccessResponse(response);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting top apps: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve top apps: %1").arg(e.what()));
    }
}

QHttpServerResponse AppUsageController::handleGetActiveApps(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("AppUsageController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET active apps request for session: %1").arg(sessionId));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Get active app usages for the session
        QList<QSharedPointer<AppUsageModel>> activeUsages = m_appUsageRepository->getActiveAppUsages(sessionUuid);

        // Prepare response array
        QJsonArray activeAppsArray;

        for (const auto &usage : activeUsages) {
            // Get application details
            auto application = m_applicationRepository->getById(usage->appId());

            QJsonObject appInfo = appUsageToJson(usage.data());

            // Add application name if available
            if (application) {
                appInfo["app_name"] = application->appName();
                appInfo["app_path"] = application->appPath();
                appInfo["is_restricted"] = application->isRestricted();
            }

            activeAppsArray.append(appInfo);
        }

        // Create response object
        QJsonObject response;
        response["session_id"] = uuidToString(sessionUuid);
        response["active_count"] = activeUsages.size();
        response["active_apps"] = activeAppsArray;

        LOG_INFO(QString("Retrieved %1 active apps for session: %2").arg(activeUsages.size()).arg(sessionId));
        return createSuccessResponse(response);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting active apps: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve active apps: %1").arg(e.what()));
    }
}

// Helper method: appUsageToJson - Creates a JSON object from an AppUsageModel
QJsonObject AppUsageController::appUsageToJson(AppUsageModel *appUsage) const
{
    QJsonObject json;
    json["usage_id"] = uuidToString(appUsage->id());
    json["session_id"] = uuidToString(appUsage->sessionId());
    json["app_id"] = uuidToString(appUsage->appId());
    json["start_time"] = appUsage->startTime().toUTC().toString();

    if (appUsage->endTime().isValid()) {
        json["end_time"] = appUsage->endTime().toUTC().toString();
    }

    json["is_active"] = appUsage->isActive();
    json["window_title"] = appUsage->windowTitle();
    json["duration_seconds"] = appUsage->duration();
    json["created_at"] = appUsage->createdAt().toUTC().toString();

    if (!appUsage->createdBy().isNull()) {
        json["created_by"] = uuidToString(appUsage->createdBy());
    }

    json["updated_at"] = appUsage->updatedAt().toUTC().toString();

    if (!appUsage->updatedBy().isNull()) {
        json["updated_by"] = uuidToString(appUsage->updatedBy());
    }

    return json;
}

// Helper method: stringToUuid - Converts a string to a QUuid
QUuid AppUsageController::stringToUuid(const QString &str) const
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

// Helper method: uuidToString - Converts a QUuid to a string
QString AppUsageController::uuidToString(const QUuid &uuid) const
{
    return uuid.toString(QUuid::WithoutBraces);
}


