#include "SessionController.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QUrlQuery>
#include "../Utils/SystemInfo.h"
#include "logger/logger.h"
#include "httpserver/response.h"

SessionController::SessionController(QObject *parent)
    : ApiControllerBase(parent)
    , m_repository(nullptr)
    , m_activityEventRepository(nullptr)
    , m_afkPeriodRepository(nullptr)
    , m_appUsageRepository(nullptr)
    , m_machineRepository(nullptr)
    , m_initialized(false)
    , m_authController(nullptr)
{
    LOG_DEBUG("SessionController created");
}

SessionController::SessionController(
    SessionRepository* sessionRepository,
    ActivityEventRepository* activityEventRepository,
    AfkPeriodRepository* afkPeriodRepository,
    AppUsageRepository* appUsageRepository,
    QObject* parent)
    : ApiControllerBase(parent)
    , m_repository(sessionRepository)
    , m_activityEventRepository(activityEventRepository)
    , m_afkPeriodRepository(afkPeriodRepository)
    , m_appUsageRepository(appUsageRepository)
    , m_machineRepository(nullptr)
    , m_initialized(false)
    , m_authController(nullptr)
{
    LOG_DEBUG("SessionController created with existing repositories");

    // Check if repositories are initialized
    if (m_repository && m_activityEventRepository &&
        m_afkPeriodRepository && m_appUsageRepository &&
        m_repository->isInitialized() && m_activityEventRepository->isInitialized() &&
        m_afkPeriodRepository->isInitialized() && m_appUsageRepository->isInitialized()) {
        m_initialized = true;
        LOG_INFO("SessionController initialized successfully");
        }
}

SessionController::~SessionController()
{
    // Only delete repositories if we created them
    if (m_repository && m_repository->parent() == this) {
        delete m_repository;
        m_repository = nullptr;
    }

    if (m_activityEventRepository && m_activityEventRepository->parent() == this) {
        delete m_activityEventRepository;
        m_activityEventRepository = nullptr;
    }

    if (m_afkPeriodRepository && m_afkPeriodRepository->parent() == this) {
        delete m_afkPeriodRepository;
        m_afkPeriodRepository = nullptr;
    }

    if (m_appUsageRepository && m_appUsageRepository->parent() == this) {
        delete m_appUsageRepository;
        m_appUsageRepository = nullptr;
    }

    LOG_DEBUG("SessionController destroyed");
}

bool SessionController::initialize()
{
    if (m_initialized) {
        LOG_WARNING("SessionController already initialized");
        return true;
    }

    LOG_DEBUG("Initializing SessionController");

    try {
        // Check if repositories are valid and initialized
        if (!m_repository) {
            LOG_ERROR("Session repository not provided");
            return false;
        }

        if (!m_activityEventRepository) {
            LOG_ERROR("ActivityEvent repository not provided");
            return false;
        }

        if (!m_afkPeriodRepository) {
            LOG_ERROR("AfkPeriod repository not provided");
            return false;
        }

        if (!m_appUsageRepository) {
            LOG_ERROR("AppUsage repository not provided");
            return false;
        }

        if (!m_repository->isInitialized()) {
            LOG_ERROR("Session repository not initialized");
            return false;
        }

        if (!m_activityEventRepository->isInitialized()) {
            LOG_ERROR("ActivityEvent repository not initialized");
            return false;
        }

        if (!m_afkPeriodRepository->isInitialized()) {
            LOG_ERROR("AfkPeriod repository not initialized");
            return false;
        }

        if (!m_appUsageRepository->isInitialized()) {
            LOG_ERROR("AppUsage repository not initialized");
            return false;
        }

        if (m_machineRepository && !m_machineRepository->isInitialized()) {
            LOG_WARNING("Machine repository provided but not initialized");
            // We don't return false here because it's optional
        }

        if (m_sessionEventRepository && m_repository) {
            m_repository->setSessionEventRepository(m_sessionEventRepository);
            LOG_DEBUG("Linked SessionEventRepository to SessionRepository");
        }

        m_initialized = true;
        LOG_INFO("SessionController initialized successfully");
        return true;
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception during SessionController initialization: %1").arg(e.what()));
        return false;
    }
}

void SessionController::setupRoutes(QHttpServer &server)
{
    if (!m_initialized) {
        LOG_ERROR("Cannot setup routes - SessionController not initialized");
        return;
    }

    LOG_INFO("Setting up SessionController routes");

    // Get all sessions
    server.route("/api/sessions", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetSessions(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get session by ID
    server.route("/api/sessions/<arg>", QHttpServerRequest::Method::Get,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetSessionById(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Create a new session
    server.route("/api/sessions", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleCreateSession(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // End a session
    server.route("/api/sessions/<arg>/end", QHttpServerRequest::Method::Post,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleEndSession(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get active session for current user
    server.route("/api/sessions/active", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetActiveSession(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get sessions by user ID
    server.route("/api/users/<arg>/sessions", QHttpServerRequest::Method::Get,
        [this](const qint64 userId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetSessionsByUserId(userId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get sessions by machine ID
    server.route("/api/machines/<arg>/sessions", QHttpServerRequest::Method::Get,
        [this](const qint64 machineId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetSessionsByMachineId(machineId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get activities for a session
    server.route("/api/sessions/<arg>/activities", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetSessionActivities(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Record activity for a session
    server.route("/api/sessions/<arg>/activities", QHttpServerRequest::Method::Post,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleRecordActivity(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Start AFK period
    server.route("/api/sessions/<arg>/afk/start", QHttpServerRequest::Method::Post,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleStartAfk(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // End AFK period
    server.route("/api/sessions/<arg>/afk/end", QHttpServerRequest::Method::Post,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleEndAfk(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get AFK periods for a session
    server.route("/api/sessions/<arg>/afk", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetAfkPeriods(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get session statistics
    server.route("/api/sessions/<arg>/stats", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetSessionStats(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get user statistics
    server.route("/api/users/<arg>/stats", QHttpServerRequest::Method::Get,
        [this](const qint64 userId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetUserStats(userId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get session chain
    server.route("/api/sessions/<arg>/chain", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetSessionChain(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    LOG_INFO("SessionController routes configured");
}

QHttpServerResponse SessionController::handleGetSessions(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET all sessions request");

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        // Parse query parameters for filtering
        QUrlQuery query(request.url().query());
        bool activeOnly = (query.queryItemValue("active", QUrl::FullyDecoded) == "true");

        QList<QSharedPointer<SessionModel>> sessions;

        if (activeOnly) {
            sessions = m_repository->getActiveSessions();
        } else {
            sessions = m_repository->getAll();
        }

        QJsonArray sessionsArray;
        for (const auto &session : sessions) {
            sessionsArray.append(sessionToJson(session.data()));
        }

        // Use base class response method
        return createSuccessResponse(sessionsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting sessions: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve sessions: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleGetSessionById(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session by ID request: %1").arg(id));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionId = stringToUuid(QString::number(id));
        auto session = m_repository->getById(sessionId);

        if (!session) {
            LOG_WARNING(QString("Session not found with ID: %1").arg(id));
            return Http::Response::notFound("Session not found");
        }

        // Use base class response method
        return createSuccessResponse(sessionToJson(session.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session: %1").arg(e.what()));
    }
}

// Handling session creation with one session per day approach
QHttpServerResponse SessionController::handleCreateSession(const QHttpServerRequest& request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing CREATE session request");

    // Check authentication using base class method
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

        // Extract username from the request data or user data
        QString username;
        if (json.contains("username") && !json["username"].toString().isEmpty()) {
            username = json["username"].toString();
            LOG_DEBUG(QString("Username found in JSON: %1").arg(username));
        }
        else if (userData.contains("username") && !userData["username"].toString().isEmpty()) {
            username = userData["username"].toString();
            LOG_DEBUG(QString("Username found in userData: %1").arg(username));
        }
        else if (userData.contains("name") && !userData["name"].toString().isEmpty()) {
            username = userData["name"].toString();
            LOG_DEBUG(QString("Using name as username: %1").arg(username));
        }
        else if (json.contains("user_id") && !json["user_id"].toString().isEmpty()) {
            username = json["user_id"].toString(); // Sometimes user_id field contains the username
            LOG_DEBUG(QString("Using user_id field as username: %1").arg(username));
        }

        if (username.isEmpty()) {
            LOG_ERROR("No username provided for session creation");
            return createErrorResponse("Username is required", QHttpServerResponder::StatusCode::BadRequest);
        }

        // Validate or create the user
        auto user = m_authController ?
            m_authController->validateAndGetUserForTracking(username) : nullptr;

        if (!user) {
            LOG_ERROR(QString("Failed to validate or create user: %1").arg(username));
            return createErrorResponse("User validation failed", QHttpServerResponder::StatusCode::UnprocessableEntity);
        }

        LOG_DEBUG(QString("Creating session for user: %1 (ID: %2)")
            .arg(user->name(), user->id().toString(QUuid::WithoutBraces)));

        // Get machine information
        if (!json.contains("machine_id") || json["machine_id"].toString().isEmpty()) {
            LOG_ERROR("No machine_id provided for session creation");
            return createErrorResponse("machine_id is required", QHttpServerResponder::StatusCode::BadRequest);
        }

        // Get machine information from request
        auto machineId = QUuid(json["machine_id"].toString());

        // Validate that the machine exists without fetching the whole object
        if (m_machineRepository != nullptr && m_machineRepository->isInitialized()) {
            bool exists = m_machineRepository->exists(machineId);
            if (!exists) {
                LOG_ERROR(QString("Machine with ID %1 not found").arg(machineId.toString()));
                return createErrorResponse("Machine not found", QHttpServerResponder::StatusCode::NotFound);
            }
        }

        // Get current date and time
        QDateTime currentDateTime = QDateTime::currentDateTimeUtc();

        // Process IP address
        QHostAddress ipAddress;
        if (json.contains("ip_address") && !json["ip_address"].toString().isEmpty()) {
            ipAddress = QHostAddress(json["ip_address"].toString());
        } else {
            // Try to get from request
            ipAddress = QHostAddress(request.remoteAddress().toString());
        }

        // Extract session data if provided
        QJsonObject sessionData;
        if (json.contains("session_data") && json["session_data"].isObject()) {
            sessionData = json["session_data"].toObject();
        }

        // Get optional parameters
        bool isRemote = json.contains("is_remote") ? json["is_remote"].toBool() : false;
        QString terminalSessionId;
        if (json.contains("terminal_session_id") && !json["terminal_session_id"].toString().isEmpty()) {
            terminalSessionId = json["terminal_session_id"].toString();
        }

        // Use the repository to create or reuse a session, all in one transaction
        auto session = m_repository->createOrReuseSessionWithTransaction(
            user->id(),
            machineId,
            currentDateTime,
            ipAddress,
            sessionData,
            isRemote,
            terminalSessionId
        );

        if (!session) {
            LOG_ERROR("Failed to create or reuse session");
            return createErrorResponse("Failed to create session", QHttpServerResponder::StatusCode::InternalServerError);
        }
        LOG_DEBUG(QString("New Session created with ID: %1").arg(session->id().toString()));

        // Explicitly check if login event was created, and create it if missing
        if (m_sessionEventRepository && m_sessionEventRepository->isInitialized()) {
            if (!m_repository->hasLoginEvent(session->id(), currentDateTime, m_sessionEventRepository)) {
                LOG_WARNING(QString("No login event found for new session %1. Creating one as fallback.")
                           .arg(session->id().toString()));

                SessionEventModel* event = new SessionEventModel();
                // event->setId(QUuid::createUuid());
                event->setSessionId(session->id());
                event->setEventType(EventTypes::SessionEventType::Login);
                event->setEventTime(currentDateTime);
                event->setUserId(user->id());
                event->setMachineId(machineId);
                event->setIsRemote(isRemote);

                if (!terminalSessionId.isEmpty()) {
                    event->setTerminalSessionId(terminalSessionId);
                }

                // Add additional context
                QJsonObject eventData;
                eventData["reason"] = "fallback_creation";
                eventData["auto_generated"] = true;
                event->setEventData(eventData);

                // Set metadata
                event->setCreatedBy(user->id());
                event->setUpdatedBy(user->id());
                event->setCreatedAt(currentDateTime);
                event->setUpdatedAt(currentDateTime);

                bool eventSuccess = m_sessionEventRepository->save(event);
                if (!eventSuccess) {
                    LOG_ERROR(QString("Still failed to create login event for session: %1")
                             .arg(session->id().toString()));
                    // Continue anyway as we at least have the session
                } else {
                    LOG_INFO(QString("Fallback login event created for session: %1")
                            .arg(session->id().toString()));
                }
                delete event;
            } else {
                LOG_INFO(QString("Login event already exists for session: %1")
                        .arg(session->id().toString()));
            }
        } else {
            LOG_WARNING("SessionEventRepository not available or not initialized - cannot verify login event");
        }

        // Double-check to make sure session event was really created (final verification)
        if (m_sessionEventRepository && m_sessionEventRepository->isInitialized()) {
            QList<QSharedPointer<SessionEventModel>> events = m_sessionEventRepository->getBySessionId(session->id());
            LOG_INFO(QString("Session %1 has %2 events associated with it")
                     .arg(session->id().toString())
                     .arg(events.size()));
        }

        // Determine if this is a new session or an existing one for status code
        bool isNewSession = (session->createdAt().secsTo(currentDateTime) < 5);
        QHttpServerResponder::StatusCode statusCode = isNewSession ?
            QHttpServerResponder::StatusCode::Created :
            QHttpServerResponder::StatusCode::Ok;

        LOG_INFO(QString("%1 session %2 for user %3 on machine %4")
            .arg(isNewSession ? "Created new" : "Using existing")
            .arg(session->id().toString(), user->name(), machineId.toString()));

        return createSuccessResponse(sessionToJson(session.data()), statusCode);
    }
    catch (const std::exception& e) {
        LOG_ERROR(QString("Exception creating session: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to create session: %1").arg(e.what()));
    }
}

// Handling session end
QHttpServerResponse SessionController::handleEndSession(const qint64 id, const QHttpServerRequest& request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing END session request: %1").arg(id));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionId = stringToUuid(QString::number(id));
        auto session = m_repository->getById(sessionId);

        if (!session) {
            LOG_WARNING(QString("Session not found with ID: %1").arg(id));
            return Http::Response::notFound("Session not found");
        }

        // End the session
        QDateTime logoutTime = QDateTime::currentDateTimeUtc();
        session->setLogoutTime(logoutTime);
        session->setUpdatedAt(logoutTime);
        session->setUpdatedBy(QUuid(userData["id"].toString()));

        bool success = m_repository->update(session.data());

        if (!success) {
            LOG_ERROR(QString("Failed to end session: %1").arg(id));
            return createErrorResponse("Failed to end session", QHttpServerResponder::StatusCode::InternalServerError);
        }

        // Create a logout session event
        if (m_sessionEventRepository && m_sessionEventRepository->isInitialized()) {
            SessionEventModel* event = new SessionEventModel();
            event->setSessionId(session->id());
            event->setEventType(EventTypes::SessionEventType::Logout);
            event->setEventTime(logoutTime);
            event->setUserId(session->userId());
            event->setMachineId(session->machineId());

            // Parse request for optional parameters
            bool ok;
            QJsonObject json = extractJsonFromRequest(request, ok);
            if (ok) {
                // Check if this is a remote session
                if (json.contains("is_remote")) {
                    event->setIsRemote(json["is_remote"].toBool());
                }

                // Check for terminal session ID
                if (json.contains("terminal_session_id") && !json["terminal_session_id"].toString().isEmpty()) {
                    event->setTerminalSessionId(json["terminal_session_id"].toString());
                }

                // Additional data for logout
                if (json.contains("logout_reason") && !json["logout_reason"].toString().isEmpty()) {
                    QJsonObject eventData;
                    eventData["reason"] = json["logout_reason"].toString();
                    event->setEventData(eventData);
                }
            }

            // Set metadata
            QUuid userId = QUuid(userData["id"].toString());
            event->setCreatedBy(userId);
            event->setUpdatedBy(userId);
            event->setCreatedAt(logoutTime);
            event->setUpdatedAt(logoutTime);

            bool eventSuccess = m_sessionEventRepository->save(event);

            if (eventSuccess) {
                LOG_INFO(QString("Logout event recorded for session: %1").arg(session->id().toString()));
            }
            else {
                LOG_WARNING(QString("Failed to record logout event for session: %1").arg(session->id().toString()));
            }

            delete event;
        }
        else {
            LOG_WARNING("Session event repository not available - logout event not recorded");
        }

        // End any active AFK periods
        auto activeAfkPeriods = m_afkPeriodRepository->getActiveAfkPeriods(sessionId);
        for (const auto& afk : activeAfkPeriods) {
            m_afkPeriodRepository->endAfkPeriod(afk->id(), logoutTime);
        }

        // End any active app usages
        auto activeAppUsages = m_appUsageRepository->getActiveAppUsages(sessionId);
        for (const auto& appUsage : activeAppUsages) {
            m_appUsageRepository->endAppUsage(appUsage->id(), logoutTime);
        }

        LOG_INFO(QString("Session ended successfully: %1").arg(id));
        return createSuccessResponse(sessionToJson(session.data()));
    }
    catch (const std::exception& e) {
        LOG_ERROR(QString("Exception ending session: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to end session: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleGetActiveSession(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET active session request");

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid userId = QUuid(userData["id"].toString());
        LOG_DEBUG(userId.toString());
        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        LOG_DEBUG(QString("Parsed JSON: %1").arg(QString::fromUtf8(QJsonDocument(json).toJson())));

        // Get machine ID from query parameters if provided
        QUrlQuery query(request.url().query());
        QString machineIdStr = query.queryItemValue("machine_id", QUrl::FullyDecoded);
        QUuid machineId;

        if (!machineIdStr.isEmpty()) {
            machineId = stringToUuid(machineIdStr);
        } else {
            // Use the current machine ID from system info
            QString uniqueId = SystemInfo::getMachineUniqueId();
            // In a real app, you would look up the machine ID by unique ID
            // For this example, we'll just use a placeholder UUID
            // machineId = QUuid::createUuid();
        }

        auto session = m_repository->getActiveSessionForUser(userId, machineId);

        if (!session) {
            LOG_WARNING("No active session found");
            return Http::Response::notFound("No active session found");
        }

        LOG_INFO(QString("Active session found: %1").arg(session->id().toString()));
        return createSuccessResponse(sessionToJson(session.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting active session: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve active session: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleGetSessionsByUserId(const qint64 userId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET sessions by user ID: %1").arg(userId));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid userUuid = stringToUuid(QString::number(userId));

        // Parse query parameters for filtering
        QUrlQuery query(request.url().query());
        bool activeOnly = (query.queryItemValue("active", QUrl::FullyDecoded) == "true");

        QList<QSharedPointer<SessionModel>> sessions = m_repository->getByUserId(userUuid, activeOnly);

        QJsonArray sessionsArray;
        for (const auto &session : sessions) {
            sessionsArray.append(sessionToJson(session.data()));
        }

        LOG_INFO(QString("Retrieved %1 sessions for user %2").arg(sessions.size()).arg(userId));
        return createSuccessResponse(sessionsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting sessions by user ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve sessions: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleGetSessionsByMachineId(const qint64 machineId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET sessions by machine ID: %1").arg(machineId));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid machineUuid = stringToUuid(QString::number(machineId));

        // Parse query parameters for filtering
        QUrlQuery query(request.url().query());
        bool activeOnly = (query.queryItemValue("active", QUrl::FullyDecoded) == "true");

        QList<QSharedPointer<SessionModel>> sessions = m_repository->getByMachineId(machineUuid, activeOnly);

        QJsonArray sessionsArray;
        for (const auto &session : sessions) {
            sessionsArray.append(sessionToJson(session.data()));
        }

        LOG_INFO(QString("Retrieved %1 sessions for machine %2").arg(sessions.size()).arg(machineId));
        return createSuccessResponse(sessionsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting sessions by machine ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve sessions: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleGetSessionActivities(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session activities: %1").arg(sessionId));

    // Check authentication using base class method
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
        int offset = query.queryItemValue("offset", QUrl::FullyDecoded).toInt();

        QList<QSharedPointer<ActivityEventModel>> activities = m_activityEventRepository->getBySessionId(sessionUuid, limit, offset);

        QJsonArray activitiesArray;
        for (const auto &activity : activities) {
            activitiesArray.append(activityEventToJson(activity.data()));
        }

        LOG_INFO(QString("Retrieved %1 activities for session %2").arg(activities.size()).arg(sessionId));
        return createSuccessResponse(activitiesArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session activities: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session activities: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleRecordActivity(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing RECORD activity for session: %1").arg(sessionId));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));
        auto session = m_repository->getById(sessionUuid);

        if (!session) {
            LOG_WARNING(QString("Session not found with ID: %1").arg(sessionId));
            return Http::Response::notFound("Session not found");
        }

        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data");
            return Http::Response::badRequest("Invalid JSON data");
        }

        // Create new activity event
        ActivityEventModel *event = new ActivityEventModel();
        event->setSessionId(sessionUuid);

        // Set app ID if provided
        if (json.contains("app_id") && !json["app_id"].toString().isEmpty()) {
            event->setAppId(QUuid(json["app_id"].toString()));
        }

        // Set event type
        if (json.contains("event_type") && !json["event_type"].toString().isEmpty()) {
            QString eventTypeStr = json["event_type"].toString();
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

        // Set event time
        if (json.contains("event_time") && !json["event_time"].toString().isEmpty()) {
            event->setEventTime(QDateTime::fromString(json["event_time"].toString(), Qt::ISODate));
        } else {
            event->setEventTime(QDateTime::currentDateTimeUtc());
        }

        // Set event data
        if (json.contains("event_data") && json["event_data"].isObject()) {
            event->setEventData(json["event_data"].toObject());
        }

        // Set metadata
        QUuid userId = QUuid(userData["id"].toString());
        event->setCreatedBy(userId);
        event->setUpdatedBy(userId);

        bool success = m_activityEventRepository->save(event);

        if (!success) {
            delete event;
            LOG_ERROR("Failed to record activity");
            return createErrorResponse("Failed to record activity", QHttpServerResponder::StatusCode::InternalServerError);
        }

        QJsonObject response = activityEventToJson(event);
        LOG_INFO(QString("Activity recorded successfully: %1").arg(event->id().toString()));
        delete event;

        return createSuccessResponse(response, QHttpServerResponder::StatusCode::Created);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception recording activity: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to record activity: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleStartAfk(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing START AFK for session: %1").arg(sessionId));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));
        auto session = m_repository->getById(sessionUuid);

        if (!session) {
            LOG_WARNING(QString("Session not found with ID: %1").arg(sessionId));
            return Http::Response::notFound("Session not found");
        }

        // Check if there's already an active AFK period
        auto activeAfkPeriods = m_afkPeriodRepository->getActiveAfkPeriods(sessionUuid);
        if (!activeAfkPeriods.isEmpty()) {
            LOG_WARNING("An AFK period is already active for this session");
            return createErrorResponse("An AFK period is already active for this session");
        }

        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);

        // Create new AFK period
        AfkPeriodModel *afkPeriod = new AfkPeriodModel();
        afkPeriod->setSessionId(sessionUuid);

        // Set start time
        if (ok && json.contains("start_time") && !json["start_time"].toString().isEmpty()) {
            afkPeriod->setStartTime(QDateTime::fromString(json["start_time"].toString(), Qt::ISODate));
        } else {
            afkPeriod->setStartTime(QDateTime::currentDateTimeUtc());
        }

        // Set metadata
        QUuid userId = QUuid(userData["id"].toString());
        afkPeriod->setCreatedBy(userId);
        afkPeriod->setUpdatedBy(userId);

        bool success = m_afkPeriodRepository->save(afkPeriod);

        if (!success) {
            delete afkPeriod;
            LOG_ERROR("Failed to start AFK period");
            return createErrorResponse("Failed to start AFK period", QHttpServerResponder::StatusCode::InternalServerError);
        }

        // Also record an activity event for this
        ActivityEventModel *event = new ActivityEventModel();
        event->setSessionId(sessionUuid);
        event->setEventType(EventTypes::ActivityEventType::AfkStart);
        event->setEventTime(afkPeriod->startTime());
        event->setCreatedBy(userId);
        event->setUpdatedBy(userId);

        m_activityEventRepository->save(event);
        delete event;

        QJsonObject response = afkPeriodToJson(afkPeriod);
        LOG_INFO(QString("AFK period started successfully: %1").arg(afkPeriod->id().toString()));
        delete afkPeriod;

        return createSuccessResponse(response, QHttpServerResponder::StatusCode::Created);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception starting AFK period: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to start AFK period: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleEndAfk(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing END AFK for session: %1").arg(sessionId));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));
        auto session = m_repository->getById(sessionUuid);

        if (!session) {
            LOG_WARNING(QString("Session not found with ID: %1").arg(sessionId));
            return Http::Response::notFound("Session not found");
        }

        // Get active AFK periods
        auto activeAfkPeriods = m_afkPeriodRepository->getActiveAfkPeriods(sessionUuid);
        if (activeAfkPeriods.isEmpty()) {
            LOG_WARNING("No active AFK period found for this session");
            return Http::Response::notFound("No active AFK period found for this session");
        }

        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);

        // Use the first active AFK period
        QSharedPointer<AfkPeriodModel> afkPeriod = activeAfkPeriods.first();

        // Set end time
        QDateTime endTime;
        if (ok && json.contains("end_time") && !json["end_time"].toString().isEmpty()) {
            endTime = QDateTime::fromString(json["end_time"].toString(), Qt::ISODate);
        } else {
            endTime = QDateTime::currentDateTimeUtc();
        }

        bool success = m_afkPeriodRepository->endAfkPeriod(afkPeriod->id(), endTime);

        if (!success) {
            LOG_ERROR("Failed to end AFK period");
            return createErrorResponse("Failed to end AFK period", QHttpServerResponder::StatusCode::InternalServerError);
        }

        // Reload the AFK period to get the updated data
        afkPeriod = m_afkPeriodRepository->getById(afkPeriod->id());

        // Also record an activity event for this
        ActivityEventModel *event = new ActivityEventModel();
        event->setSessionId(sessionUuid);
        event->setEventType(EventTypes::ActivityEventType::AfkEnd);
        event->setEventTime(endTime);
        event->setCreatedBy(QUuid(userData["id"].toString()));
        event->setUpdatedBy(QUuid(userData["id"].toString()));

        m_activityEventRepository->save(event);
        delete event;

        LOG_INFO(QString("AFK period ended successfully: %1").arg(afkPeriod->id().toString()));
        return createSuccessResponse(afkPeriodToJson(afkPeriod.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception ending AFK period: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to end AFK period: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleGetAfkPeriods(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET AFK periods for session: %1").arg(sessionId));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));
        auto session = m_repository->getById(sessionUuid);

        if (!session) {
            LOG_WARNING(QString("Session not found with ID: %1").arg(sessionId));
            return Http::Response::notFound("Session not found");
        }

        // Get all AFK periods for this session
        QList<QSharedPointer<AfkPeriodModel>> afkPeriods = m_afkPeriodRepository->getBySessionId(sessionUuid);

        QJsonArray afkPeriodsArray;
        for (const auto &afkPeriod : afkPeriods) {
            afkPeriodsArray.append(afkPeriodToJson(afkPeriod.data()));
        }

        LOG_INFO(QString("Retrieved %1 AFK periods for session %2").arg(afkPeriods.size()).arg(sessionId));
        return createSuccessResponse(afkPeriodsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting AFK periods: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve AFK periods: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleGetSessionStats(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session stats: %1").arg(sessionId));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));
        auto session = m_repository->getById(sessionUuid);

        if (!session) {
            LOG_WARNING(QString("Session not found with ID: %1").arg(sessionId));
            return Http::Response::notFound("Session not found");
        }

        // Get session summary stats
        QJsonObject stats;

        // Basic session info
        stats["session_id"] = uuidToString(session->id());
        stats["user_id"] = uuidToString(session->userId());
        stats["login_time"] = session->loginTime().toString(Qt::ISODate);

        if (session->logoutTime().isValid()) {
            stats["logout_time"] = session->logoutTime().toString(Qt::ISODate);
            stats["active"] = false;
        } else {
            stats["active"] = true;
        }

        // Calculate duration
        QDateTime endTime = session->logoutTime().isValid() ? session->logoutTime() : QDateTime::currentDateTimeUtc();
        stats["duration_seconds"] = session->loginTime().secsTo(endTime);

        // AFK stats
        QJsonObject afkSummary = m_afkPeriodRepository->getAfkSummary(sessionUuid);
        stats["afk_stats"] = afkSummary;

        // Calculate actual active time (total duration - afk time)
        double totalAfkSeconds = afkSummary["total_afk_seconds"].toDouble();
        stats["active_seconds"] = stats["duration_seconds"].toDouble() - totalAfkSeconds;

        if (stats["duration_seconds"].toDouble() > 0) {
            stats["afk_percentage"] = (totalAfkSeconds / stats["duration_seconds"].toDouble()) * 100.0;
        } else {
            stats["afk_percentage"] = 0.0;
        }

        // Activity stats
        QJsonObject activitySummary = m_activityEventRepository->getActivitySummary(sessionUuid);
        stats["activity_stats"] = activitySummary;

        // App usage stats
        QJsonObject appUsageSummary = m_appUsageRepository->getAppUsageSummary(sessionUuid);
        stats["app_usage_stats"] = appUsageSummary;

        // Top apps
        stats["top_apps"] = m_appUsageRepository->getTopApps(sessionUuid, 5);

        LOG_INFO(QString("Session stats retrieved for session %1").arg(sessionId));
        return createSuccessResponse(stats);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session stats: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session stats: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleGetUserStats(const qint64 userId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET user stats: %1").arg(userId));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid userUuid = stringToUuid(QString::number(userId));

        // Parse query parameters
        QUrlQuery query(request.url().query());
        QString startDateStr = query.queryItemValue("start_date", QUrl::FullyDecoded);
        QString endDateStr = query.queryItemValue("end_date", QUrl::FullyDecoded);

        QDateTime startDate = startDateStr.isEmpty() ? QDateTime::currentDateTimeUtc().addDays(-30) : QDateTime::fromString(startDateStr, Qt::ISODate);
        QDateTime endDate = endDateStr.isEmpty() ? QDateTime::currentDateTimeUtc() : QDateTime::fromString(endDateStr, Qt::ISODate);

        QJsonObject stats = m_repository->getUserSessionStats(userUuid, startDate, endDate);

        LOG_INFO(QString("User stats retrieved for user %1").arg(userId));
        return createSuccessResponse(stats);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting user stats: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve user stats: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionController::handleGetSessionChain(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session chain: %1").arg(sessionId));

    // Check authentication using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));
        auto session = m_repository->getById(sessionUuid);

        if (!session) {
            LOG_WARNING(QString("Session not found with ID: %1").arg(sessionId));
            return Http::Response::notFound("Session not found");
        }

        // Get session chain
        QList<QSharedPointer<SessionModel>> chainSessions = m_repository->getSessionChain(sessionUuid);

        // Get chain statistics
        QJsonObject chainStats = m_repository->getSessionChainStats(sessionUuid);

        // Create response with both chain sessions and stats
        QJsonArray sessionsArray;
        for (const auto &chainSession : chainSessions) {
            sessionsArray.append(sessionToJson(chainSession.data()));
        }

        QJsonObject response;
        response["chain_stats"] = chainStats;
        response["sessions"] = sessionsArray;

        LOG_INFO(QString("Session chain retrieved for session %1").arg(sessionId));
        return createSuccessResponse(response);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session chain: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session chain: %1").arg(e.what()));
    }
}

QJsonObject SessionController::sessionToJson(SessionModel *session) const
{
    QJsonObject json;
    json["session_id"] = uuidToString(session->id());
    json["user_id"] = uuidToString(session->userId());

    json["login_time"] = session->loginTime().toString(Qt::ISODate);

    if (session->logoutTime().isValid()) {
        json["logout_time"] = session->logoutTime().toString(Qt::ISODate);
    }

    json["machine_id"] = uuidToString(session->machineId());
    json["ip_address"] = session->ipAddress().toString();
    json["session_data"] = session->sessionData();
    json["created_at"] = session->createdAt().toString(Qt::ISODate);

    if (!session->createdBy().isNull()) {
        json["created_by"] = uuidToString(session->createdBy());
    }

    json["updated_at"] = session->updatedAt().toString(Qt::ISODate);

    if (!session->updatedBy().isNull()) {
        json["updated_by"] = uuidToString(session->updatedBy());
    }

    // Session continuity info
    if (!session->continuedFromSession().isNull()) {
        json["continued_from_session"] = uuidToString(session->continuedFromSession());
    }

    if (!session->continuedBySession().isNull()) {
        json["continued_by_session"] = uuidToString(session->continuedBySession());
    }

    if (session->previousSessionEndTime().isValid()) {
        json["previous_session_end_time"] = session->previousSessionEndTime().toString(Qt::ISODate);
    }

    json["time_since_previous_session"] = session->timeSincePreviousSession();
    json["is_active"] = session->isActive();
    json["duration_seconds"] = session->duration();

    return json;
}

QJsonObject SessionController::afkPeriodToJson(AfkPeriodModel *afkPeriod) const
{
    QJsonObject json;
    json["afk_id"] = uuidToString(afkPeriod->id());
    json["session_id"] = uuidToString(afkPeriod->sessionId());

    json["start_time"] = afkPeriod->startTime().toString(Qt::ISODate);

    if (afkPeriod->endTime().isValid()) {
        json["end_time"] = afkPeriod->endTime().toString(Qt::ISODate);
    }

    json["is_active"] = afkPeriod->isActive();
    json["duration_seconds"] = afkPeriod->duration();
    json["created_at"] = afkPeriod->createdAt().toString(Qt::ISODate);

    if (!afkPeriod->createdBy().isNull()) {
        json["created_by"] = uuidToString(afkPeriod->createdBy());
    }

    json["updated_at"] = afkPeriod->updatedAt().toString(Qt::ISODate);

    if (!afkPeriod->updatedBy().isNull()) {
        json["updated_by"] = uuidToString(afkPeriod->updatedBy());
    }

    return json;
}

QJsonObject SessionController::activityEventToJson(ActivityEventModel *event) const
{
    QJsonObject json;
    json["event_id"] = uuidToString(event->id());
    json["session_id"] = uuidToString(event->sessionId());

    if (!event->appId().isNull()) {
        json["app_id"] = uuidToString(event->appId());
    }

    // Convert event type enum to string
    switch (event->eventType()) {
    case EventTypes::ActivityEventType::MouseClick:
        json["event_type"] = "mouse_click";
        break;
    case EventTypes::ActivityEventType::MouseMove:
        json["event_type"] = "mouse_move";
        break;
    case EventTypes::ActivityEventType::Keyboard:
        json["event_type"] = "keyboard";
        break;
    case EventTypes::ActivityEventType::AfkStart:
        json["event_type"] = "afk_start";
        break;
    case EventTypes::ActivityEventType::AfkEnd:
        json["event_type"] = "afk_end";
        break;
    case EventTypes::ActivityEventType::AppFocus:
        json["event_type"] = "app_focus";
        break;
    case EventTypes::ActivityEventType::AppUnfocus:
        json["event_type"] = "app_unfocus";
        break;
    default:
        json["event_type"] = "unknown";
        break;
    }

    json["event_time"] = event->eventTime().toString(Qt::ISODate);
    json["event_data"] = event->eventData();
    json["created_at"] = event->createdAt().toString(Qt::ISODate);

    if (!event->createdBy().isNull()) {
        json["created_by"] = uuidToString(event->createdBy());
    }

    json["updated_at"] = event->updatedAt().toString(Qt::ISODate);

    if (!event->updatedBy().isNull()) {
        json["updated_by"] = uuidToString(event->updatedBy());
    }

    return json;
}

QJsonObject SessionController::extractJsonFromRequest(const QHttpServerRequest &request, bool &ok)
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
    LOG_DEBUG(QString("Extracted JSON: %1").arg(body));
    return doc.object();
}

bool SessionController::hasOverlappingSession(const QUuid &userId, const QUuid &machineId)
{
    LOG_DEBUG(QString("Checking for overlapping sessions for user ID: %1 and machine ID: %2")
              .arg(userId.toString(), machineId.toString()));

    if (!m_initialized) {
        return false;
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["machine_id"] = machineId.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT COUNT(*) as session_count FROM sessions "
        "WHERE user_id = :user_id AND machine_id = :machine_id AND logout_time IS NULL";

    auto result = m_repository->executeSingleSelectQuery(
        query,
        params
    );

    if (result) {
        int count = result->property("session_count").toInt();
        LOG_INFO(QString("Found %1 overlapping sessions for user ID: %2 and machine ID: %3")
                .arg(count)
                .arg(userId.toString(), machineId.toString()));
        return count > 0;
    }

    return false;
}

bool SessionController::endAllActiveSessions(const QUuid &userId, const QUuid &machineId)
{
    LOG_DEBUG(QString("Ending all active sessions for user ID: %1 and machine ID: %2")
              .arg(userId.toString(), machineId.toString()));

    if (!m_initialized) {
        return false;
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["machine_id"] = machineId.toString(QUuid::WithoutBraces);
    params["logout_time"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QString query =
        "UPDATE sessions SET "
        "logout_time = :logout_time, "
        "updated_at = :logout_time "
        "WHERE user_id = :user_id AND machine_id = :machine_id AND logout_time IS NULL";

    bool success = m_repository->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("Successfully ended all active sessions for user ID: %1 and machine ID: %2")
                .arg(userId.toString(), machineId.toString()));
    } else {
        LOG_ERROR(QString("Failed to end active sessions for user ID: %1 and machine ID: %2")
                .arg(userId.toString(), machineId.toString()));
    }

    return success;
}

// Helper method to record session events
bool SessionController::recordSessionEvent(
    const QUuid& sessionId,
    EventTypes::SessionEventType eventType,
    const QDateTime& eventTime,
    const QUuid& userId,
    const QUuid& machineId,
    bool isRemote,
    const QString& terminalSessionId)
{
    if (!m_sessionEventRepository || !m_sessionEventRepository->isInitialized()) {
        LOG_WARNING("Session event repository not available or not initialized");
        return false;
    }

    SessionEventModel* event = new SessionEventModel();
    event->setSessionId(sessionId);
    event->setEventType(eventType);
    event->setEventTime(eventTime);
    event->setUserId(userId);
    event->setMachineId(machineId);
    event->setIsRemote(isRemote);

    if (!terminalSessionId.isEmpty()) {
        event->setTerminalSessionId(terminalSessionId);
    }

    // Set metadata
    event->setCreatedBy(userId);
    event->setUpdatedBy(userId);

    bool success = m_sessionEventRepository->save(event);

    if (success) {
        LOG_INFO(QString("Recorded session event: %1 for session %2")
            .arg(event->id().toString(), sessionId.toString()));
    }
    else {
        LOG_ERROR(QString("Failed to record session event for session %1")
            .arg(sessionId.toString()));
    }

    delete event;
    return success;
}

// Handle day change - call this at the beginning of the day or when detecting a day change
QHttpServerResponse SessionController::handleDayChange(const QUuid& userId, const QUuid& machineId)
{
    if (!m_initialized) {
        LOG_ERROR("SessionController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing day change for user %1 on machine %2")
             .arg(userId.toString(), machineId.toString()));

    try {
        // Get the current active session
        auto activeSession = m_repository->getActiveSessionForUser(userId, machineId);

        if (!activeSession) {
            LOG_INFO("No active session found for day change");
            return createSuccessResponse(QJsonObject{{"message", "No active session found"}});
        }

        QDateTime now = QDateTime::currentDateTimeUtc();
        QDate sessionDate = activeSession->loginTime().date();
        QDate currentDate = now.date();

        if (sessionDate == currentDate) {
            LOG_INFO("Session already belongs to current day - no day change needed");
            return createSuccessResponse(QJsonObject{{"message", "Session already on current day"}});
        }

        // End the previous day's session
        QDateTime endOfDay(sessionDate, QTime(23, 59, 59, 999), Qt::UTC);
        activeSession->setLogoutTime(endOfDay);
        activeSession->setUpdatedAt(now);
        activeSession->setUpdatedBy(userId);

        bool updateSuccess = m_repository->update(activeSession.data());

        if (!updateSuccess) {
            LOG_ERROR("Failed to end previous day's session");
            return createErrorResponse("Failed to end previous day's session",
                                      QHttpServerResponder::StatusCode::InternalServerError);
        }

        // Create a logout session event
        if (m_sessionEventRepository && m_sessionEventRepository->isInitialized()) {
            SessionEventModel* logoutEvent = new SessionEventModel();
            logoutEvent->setSessionId(activeSession->id());
            logoutEvent->setEventType(EventTypes::SessionEventType::Logout);
            logoutEvent->setEventTime(endOfDay);
            logoutEvent->setUserId(userId);
            logoutEvent->setMachineId(machineId);

            // Set metadata
            logoutEvent->setCreatedBy(userId);
            logoutEvent->setUpdatedBy(userId);
            logoutEvent->setCreatedAt(now);
            logoutEvent->setUpdatedAt(now);

            // Add event data indicating this was a day change logout
            QJsonObject eventData;
            eventData["reason"] = "day_change";
            eventData["auto_generated"] = true;
            logoutEvent->setEventData(eventData);

            bool logoutEventSuccess = m_sessionEventRepository->save(logoutEvent);

            if (logoutEventSuccess) {
                LOG_INFO(QString("Logout event recorded for day change: %1").arg(activeSession->id().toString()));
            } else {
                LOG_WARNING(QString("Failed to record logout event for day change: %1").arg(activeSession->id().toString()));
            }

            delete logoutEvent;
        } else {
            LOG_WARNING("Session event repository not available - logout event not recorded");
        }

        // End any active AFK periods
        auto activeAfkPeriods = m_afkPeriodRepository->getActiveAfkPeriods(activeSession->id());
        for (const auto &afk : activeAfkPeriods) {
            m_afkPeriodRepository->endAfkPeriod(afk->id(), endOfDay);
        }

        // End any active app usages
        auto activeAppUsages = m_appUsageRepository->getActiveAppUsages(activeSession->id());
        for (const auto &appUsage : activeAppUsages) {
            m_appUsageRepository->endAppUsage(appUsage->id(), endOfDay);
        }

        // Create a new session for the current day
        QDateTime startOfDay(currentDate, QTime(0, 0, 0), Qt::UTC);

        SessionModel* newSession = new SessionModel();
        newSession->setId(QUuid::createUuid());
        newSession->setUserId(userId);
        newSession->setMachineId(machineId);
        newSession->setLoginTime(startOfDay);
        newSession->setIpAddress(activeSession->ipAddress());
        newSession->setSessionData(activeSession->sessionData());
        newSession->setContinuedFromSession(activeSession->id());
        newSession->setPreviousSessionEndTime(endOfDay);
        newSession->setTimeSincePreviousSession(1); // 1 second after midnight

        // Set metadata
        newSession->setCreatedBy(userId);
        newSession->setUpdatedBy(userId);
        newSession->setCreatedAt(now);
        newSession->setUpdatedAt(now);

        bool createSuccess = m_repository->save(newSession);

        if (!createSuccess) {
            LOG_ERROR("Failed to create new day's session");
            delete newSession;
            return createErrorResponse("Failed to create new day's session",
                                      QHttpServerResponder::StatusCode::InternalServerError);
        }

        // Create login event for the new session
        if (m_sessionEventRepository && m_sessionEventRepository->isInitialized()) {
            SessionEventModel* loginEvent = new SessionEventModel();
            loginEvent->setSessionId(newSession->id());
            loginEvent->setEventType(EventTypes::SessionEventType::Login);
            loginEvent->setEventTime(startOfDay);
            loginEvent->setUserId(userId);
            loginEvent->setMachineId(machineId);

            // Set metadata
            loginEvent->setCreatedBy(userId);
            loginEvent->setUpdatedBy(userId);
            loginEvent->setCreatedAt(now);
            loginEvent->setUpdatedAt(now);

            // Add event data indicating this was a day change login
            QJsonObject eventData;
            eventData["reason"] = "day_change";
            eventData["auto_generated"] = true;
            eventData["continued_from_session"] = activeSession->id().toString();
            loginEvent->setEventData(eventData);

            bool loginEventSuccess = m_sessionEventRepository->save(loginEvent);

            if (loginEventSuccess) {
                LOG_INFO(QString("Login event recorded for new day session: %1").arg(newSession->id().toString()));
            } else {
                LOG_WARNING(QString("Failed to record login event for new day session: %1").arg(newSession->id().toString()));
            }

            delete loginEvent;
        } else {
            LOG_WARNING("Session event repository not available - login event not recorded");
        }

        QJsonObject response = sessionToJson(newSession);
        LOG_INFO(QString("Day change handled - new session created: %1").arg(newSession->id().toString()));
        delete newSession;

        return createSuccessResponse(response);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception handling day change: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to handle day change: %1").arg(e.what()));
    }
}

