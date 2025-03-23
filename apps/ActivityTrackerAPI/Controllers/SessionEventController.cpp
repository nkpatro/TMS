#include "SessionEventController.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QUrlQuery>
#include "logger/logger.h"
#include "httpserver/response.h"

// Constructor implementations
SessionEventController::SessionEventController(QObject *parent)
    : ApiControllerBase(parent)
    , m_repository(nullptr)
    , m_initialized(false)
{
    LOG_DEBUG("SessionEventController created");
}

SessionEventController::SessionEventController(SessionEventRepository* repository, QObject *parent)
    : ApiControllerBase(parent)
    , m_repository(repository)
    , m_initialized(false)
{
    LOG_DEBUG("SessionEventController created with existing repository");

    // If a repository was provided, check if it's initialized
    if (m_repository && m_repository->isInitialized()) {
        m_initialized = true;
        LOG_INFO("SessionEventController initialized successfully");
    }
}

SessionEventController::~SessionEventController()
{
    // Only delete repository if we created it
    if (m_repository && m_repository->parent() == this) {
        delete m_repository;
        m_repository = nullptr;
    }

    LOG_DEBUG("SessionEventController destroyed");
}

bool SessionEventController::initialize()
{
    if (m_initialized) {
        LOG_WARNING("SessionEventController already initialized");
        return true;
    }

    LOG_DEBUG("Initializing SessionEventController");

    try {
        // Check if repository is valid and initialized
        if (!m_repository) {
            LOG_ERROR("SessionEvent repository not provided");
            return false;
        }

        if (!m_repository->isInitialized()) {
            LOG_ERROR("SessionEvent repository not initialized");
            return false;
        }

        m_initialized = true;
        LOG_INFO("SessionEventController initialized successfully");
        return true;
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception during SessionEventController initialization: %1").arg(e.what()));
        return false;
    }
}

void SessionEventController::setupRoutes(QHttpServer &server)
{
    if (!m_initialized) {
        LOG_ERROR("Cannot setup routes - SessionEventController not initialized");
        return;
    }

    LOG_INFO("Setting up SessionEventController routes");

    // Get all events
    server.route("/api/session-events", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEvents(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get event by ID
    server.route("/api/session-events/<arg>", QHttpServerRequest::Method::Get,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventById(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get events by session ID
    server.route("/api/sessions/<arg>/events", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventsBySessionId(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get events by type for a session
    server.route("/api/sessions/<arg>/events/type/<arg>", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QString &eventType, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventsByEventType(sessionId, eventType, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get events by time range for a session
    server.route("/api/sessions/<arg>/events/timerange", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventsByTimeRange(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get events by user ID
    server.route("/api/users/<arg>/session-events", QHttpServerRequest::Method::Get,
        [this](const qint64 userId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventsByUserId(userId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get events by machine ID
    server.route("/api/machines/<arg>/session-events", QHttpServerRequest::Method::Get,
        [this](const QString &machineId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventsByMachineId(machineId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Create event
    server.route("/api/session-events", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleCreateEvent(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Create event for session
    server.route("/api/sessions/<arg>/events", QHttpServerRequest::Method::Post,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleCreateEventForSession(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Update event
    server.route("/api/session-events/<arg>", QHttpServerRequest::Method::Put,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleUpdateEvent(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Delete event
    server.route("/api/session-events/<arg>", QHttpServerRequest::Method::Delete,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleDeleteEvent(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get event statistics for session
    server.route("/api/sessions/<arg>/events/stats", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventStats(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    LOG_INFO("SessionEventController routes configured");
}

QHttpServerResponse SessionEventController::handleGetEvents(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET all session events request");

    // Check authentication
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

        // Get all events
        QList<QSharedPointer<SessionEventModel>> events = m_repository->getAll();

        // Limit the results
        if (events.size() > limit) {
            events = events.mid(0, limit);
        }

        QJsonArray eventsArray;
        for (const auto &event : events) {
            eventsArray.append(sessionEventToJson(event.data()));
        }

        LOG_INFO(QString("Retrieved %1 session events").arg(events.size()));
        return createSuccessResponse(eventsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session events: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session events: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionEventController::handleGetEventById(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session event by ID request: %1").arg(id));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid eventId = stringToUuid(QString::number(id));
        auto event = m_repository->getById(eventId);

        if (!event) {
            LOG_WARNING(QString("Session event not found with ID: %1").arg(id));
            return Http::Response::notFound("Session event not found");
        }

        LOG_INFO(QString("Retrieved session event with ID: %1").arg(id));
        return createSuccessResponse(sessionEventToJson(event.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session event by ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session event: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionEventController::handleGetEventsBySessionId(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session events by session ID request: %1").arg(sessionId));

    // Check authentication
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

        // Get session events for the session
        QList<QSharedPointer<SessionEventModel>> events = m_repository->getBySessionId(sessionUuid, limit, offset);

        QJsonArray eventsArray;
        for (const auto &event : events) {
            eventsArray.append(sessionEventToJson(event.data()));
        }

        LOG_INFO(QString("Retrieved %1 session events for session %2").arg(events.size()).arg(sessionId));
        return createSuccessResponse(eventsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session events by session ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session events: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionEventController::handleGetEventsByEventType(const qint64 sessionId, const QString &eventType, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session events by event type: %1 for session: %2")
             .arg(eventType).arg(sessionId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Convert string event type to enum
        EventTypes::SessionEventType sessionEventType = stringToEventType(eventType);

        // Parse query parameters
        QUrlQuery query(request.url().query());
        int limit = query.queryItemValue("limit", QUrl::FullyDecoded).toInt();
        int offset = query.queryItemValue("offset", QUrl::FullyDecoded).toInt();

        // Get session events by type
        QList<QSharedPointer<SessionEventModel>> events = m_repository->getByEventType(
            sessionUuid, sessionEventType, limit, offset);

        QJsonArray eventsArray;
        for (const auto &event : events) {
            eventsArray.append(sessionEventToJson(event.data()));
        }

        LOG_INFO(QString("Retrieved %1 session events of type %2 for session %3")
                .arg(events.size()).arg(eventType).arg(sessionId));
        return createSuccessResponse(eventsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session events by event type: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session events: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionEventController::handleGetEventsByTimeRange(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session events by time range for session: %1").arg(sessionId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Parse query parameters
        QUrlQuery query(request.url().query());
        QString startTimeStr = query.queryItemValue("start_time", QUrl::FullyDecoded);
        QString endTimeStr = query.queryItemValue("end_time", QUrl::FullyDecoded);
        int limit = query.queryItemValue("limit", QUrl::FullyDecoded).toInt();
        int offset = query.queryItemValue("offset", QUrl::FullyDecoded).toInt();

        if (startTimeStr.isEmpty() || endTimeStr.isEmpty()) {
            LOG_WARNING("Missing start_time or end_time parameters");
            return createErrorResponse("Missing required parameters: start_time and end_time", QHttpServerResponder::StatusCode::BadRequest);
        }

        QDateTime startTime = QDateTime::fromString(startTimeStr, Qt::ISODate);
        QDateTime endTime = QDateTime::fromString(endTimeStr, Qt::ISODate);

        if (!startTime.isValid() || !endTime.isValid()) {
            LOG_WARNING("Invalid time format in parameters");
            return createErrorResponse("Invalid time format. Use ISO format (YYYY-MM-DDThh:mm:ss)", QHttpServerResponder::StatusCode::BadRequest);
        }

        // Get session events by time range
        QList<QSharedPointer<SessionEventModel>> events = m_repository->getByTimeRange(
            sessionUuid, startTime, endTime, limit, offset);

        QJsonArray eventsArray;
        for (const auto &event : events) {
            eventsArray.append(sessionEventToJson(event.data()));
        }

        LOG_INFO(QString("Retrieved %1 session events in time range for session %2")
                .arg(events.size()).arg(sessionId));
        return createSuccessResponse(eventsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session events by time range: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session events: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionEventController::handleGetEventsByUserId(const qint64 userId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session events by user ID request: %1").arg(userId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid userUuid = stringToUuid(QString::number(userId));

        // Parse query parameters
        QUrlQuery query(request.url().query());
        int limit = query.queryItemValue("limit", QUrl::FullyDecoded).toInt();
        int offset = query.queryItemValue("offset", QUrl::FullyDecoded).toInt();

        // Get session events for the user
        QList<QSharedPointer<SessionEventModel>> events = m_repository->getByUserId(userUuid, limit, offset);

        QJsonArray eventsArray;
        for (const auto &event : events) {
            eventsArray.append(sessionEventToJson(event.data()));
        }

        LOG_INFO(QString("Retrieved %1 session events for user %2").arg(events.size()).arg(userId));
        return createSuccessResponse(eventsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session events by user ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session events: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionEventController::handleGetEventsByMachineId(const QString &machineId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session events by machine ID request: %1").arg(machineId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        // Parse query parameters
        QUrlQuery query(request.url().query());
        int limit = query.queryItemValue("limit", QUrl::FullyDecoded).toInt();
        int offset = query.queryItemValue("offset", QUrl::FullyDecoded).toInt();

        // Get session events for the machine
        QList<QSharedPointer<SessionEventModel>> events = m_repository->getByMachineId(machineId, limit, offset);

        QJsonArray eventsArray;
        for (const auto &event : events) {
            eventsArray.append(sessionEventToJson(event.data()));
        }

        LOG_INFO(QString("Retrieved %1 session events for machine %2").arg(events.size()).arg(machineId));
        return createSuccessResponse(eventsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session events by machine ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session events: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionEventController::handleCreateEvent(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing CREATE session event request");

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        // Parse and validate request JSON
        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data in request");
            return createErrorResponse("Invalid JSON data", QHttpServerResponder::StatusCode::BadRequest);
        }

        // Validate required fields
        if (!json.contains("session_id") || json["session_id"].toString().isEmpty()) {
            LOG_WARNING("Missing required field: session_id");
            return createErrorResponse("Session ID is required", QHttpServerResponder::StatusCode::BadRequest);
        }

        // Parse and validate session ID
        QUuid sessionUuid;
        try {
            sessionUuid = QUuid(json["session_id"].toString());
            if (sessionUuid.isNull()) {
                LOG_WARNING("Invalid session ID format");
                return createErrorResponse("Invalid session ID format", QHttpServerResponder::StatusCode::BadRequest);
            }
        } catch (...) {
            LOG_WARNING("Exception parsing session ID");
            return createErrorResponse("Invalid session ID format", QHttpServerResponder::StatusCode::BadRequest);
        }

        // Create new session event model
        SessionEventModel *event = new SessionEventModel();
        event->setSessionId(sessionUuid);

        // Set event type (with default if not provided)
        if (json.contains("event_type") && !json["event_type"].toString().isEmpty()) {
            EventTypes::SessionEventType eventType = stringToEventType(json["event_type"].toString());
            event->setEventType(eventType);
            LOG_DEBUG(QString("Setting event type: %1").arg(eventTypeToString(eventType)));
        } else {
            // Default to Login if not specified
            event->setEventType(EventTypes::SessionEventType::Login);
            LOG_DEBUG("No event type specified, defaulting to Login");
        }

        // Set event time (with current time as default)
        if (json.contains("event_time") && !json["event_time"].toString().isEmpty()) {
            QDateTime eventTime = QDateTime::fromString(json["event_time"].toString(), Qt::ISODate);
            if (eventTime.isValid()) {
                event->setEventTime(eventTime);
                LOG_DEBUG(QString("Setting event time: %1").arg(eventTime.toString(Qt::ISODate)));
            } else {
                event->setEventTime(QDateTime::currentDateTimeUtc());
                LOG_WARNING("Invalid event time format, using current time");
            }
        } else {
            event->setEventTime(QDateTime::currentDateTimeUtc());
            LOG_DEBUG(QString("No event time specified, using current time: %1")
                     .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate)));
        }

        // Set user ID if provided
        if (json.contains("user_id") && !json["user_id"].toString().isEmpty()) {
            QUuid userId = QUuid(json["user_id"].toString());
            if (!userId.isNull()) {
                event->setUserId(userId);
                LOG_DEBUG(QString("Setting user ID: %1").arg(userId.toString()));
            }
        }

        // Set previous user ID if provided (for switch user events)
        if (json.contains("previous_user_id") && !json["previous_user_id"].toString().isEmpty()) {
            QUuid prevUserId = QUuid(json["previous_user_id"].toString());
            if (!prevUserId.isNull()) {
                event->setPreviousUserId(prevUserId);
                LOG_DEBUG(QString("Setting previous user ID: %1").arg(prevUserId.toString()));
            }
        }

        // Set machine ID if provided
        if (json.contains("machine_id") && !json["machine_id"].toString().isEmpty()) {
            QUuid machineId = QUuid(json["machine_id"].toString());
            if (!machineId.isNull()) {
                event->setMachineId(machineId);
                LOG_DEBUG(QString("Setting machine ID: %1").arg(machineId.toString()));
            }
        }

        // Set terminal session ID if provided
        if (json.contains("terminal_session_id") && !json["terminal_session_id"].toString().isEmpty()) {
            event->setTerminalSessionId(json["terminal_session_id"].toString());
            LOG_DEBUG(QString("Setting terminal session ID: %1").arg(json["terminal_session_id"].toString()));
        }

        // Set is_remote if provided
        if (json.contains("is_remote")) {
            event->setIsRemote(json["is_remote"].toBool());
            LOG_DEBUG(QString("Setting is_remote: %1").arg(json["is_remote"].toBool()));
        }

        // Set event data if provided
        if (json.contains("event_data")) {
            if (json["event_data"].isObject()) {
                event->setEventData(json["event_data"].toObject());
                LOG_DEBUG("Event data JSON object set");
            } else {
                LOG_WARNING("event_data must be a JSON object, ignoring field");
            }
        }

        // Set metadata properties
        QUuid userId = QUuid(userData["id"].toString());
        QDateTime currentTime = QDateTime::currentDateTimeUtc();

        event->setCreatedBy(userId);
        event->setUpdatedBy(userId);
        event->setCreatedAt(currentTime);
        event->setUpdatedAt(currentTime);

        LOG_DEBUG(QString("Setting metadata: createdBy=%1, timestamp=%2")
                 .arg(userId.toString(), currentTime.toString(Qt::ISODate)));

        // Save the event to the database
        LOG_DEBUG(QString("Attempting to save session event: sessionId=%1, eventType=%2")
                 .arg(event->sessionId().toString(), eventTypeToString(event->eventType())));

        bool success = m_repository->save(event);

        if (!success) {
            delete event; // Clean up if save fails
            LOG_ERROR("Failed to create session event: database operation failed");
            return createErrorResponse("Failed to create session event", QHttpServerResponder::StatusCode::InternalServerError);
        }

        // Prepare success response
        QJsonObject response = sessionEventToJson(event);

        // Add additional context to the response
        response["success"] = true;
        response["message"] = "Session event created successfully";
        response["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        LOG_INFO(QString("Session event created successfully: %1 (session: %2, type: %3)")
                .arg(event->id().toString(),
                    event->sessionId().toString(),
                    eventTypeToString(event->eventType())));

        delete event; // Clean up after successful operation

        // Return created response with 201 status code
        return createSuccessResponse(response, QHttpServerResponder::StatusCode::Created);
    }
    catch (const std::exception &e) {
        // Handle any unexpected exceptions
        LOG_ERROR(QString("Exception creating session event: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to create session event: %1").arg(e.what()), QHttpServerResponder::StatusCode::InternalServerError);
    }
}

QHttpServerResponse SessionEventController::handleCreateEventForSession(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing CREATE session event for session ID: %1").arg(sessionId));

    // Check authentication
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
            return createErrorResponse("Invalid JSON data", QHttpServerResponder::StatusCode::BadRequest);
        }

        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Create new session event
        SessionEventModel *event = new SessionEventModel();
        event->setSessionId(sessionUuid);

        // Set event type
        if (json.contains("event_type") && !json["event_type"].toString().isEmpty()) {
            event->setEventType(stringToEventType(json["event_type"].toString()));
        } else {
            // Default to Login if not specified
            event->setEventType(EventTypes::SessionEventType::Login);
        }

        // Set event time
        if (json.contains("event_time") && !json["event_time"].toString().isEmpty()) {
            event->setEventTime(QDateTime::fromString(json["event_time"].toString(), Qt::ISODate));
        } else {
            event->setEventTime(QDateTime::currentDateTimeUtc());
        }

        // Set user ID if provided
        if (json.contains("user_id") && !json["user_id"].toString().isEmpty()) {
            event->setUserId(QUuid(json["user_id"].toString()));
        }

        // Set previous user ID if provided (for switch user events)
        if (json.contains("previous_user_id") && !json["previous_user_id"].toString().isEmpty()) {
            event->setPreviousUserId(QUuid(json["previous_user_id"].toString()));
        }

        // Set machine ID
        if (json.contains("machine_id") && !json["machine_id"].toString().isEmpty()) {
            event->setMachineId(QUuid(json["machine_id"].toString()));
        }

        // Set terminal session ID
        if (json.contains("terminal_session_id") && !json["terminal_session_id"].toString().isEmpty()) {
            event->setTerminalSessionId(json["terminal_session_id"].toString());
        }

        // Set is_remote
        if (json.contains("is_remote")) {
            event->setIsRemote(json["is_remote"].toBool());
        }

        // Set event data
        if (json.contains("event_data") && json["event_data"].isObject()) {
            event->setEventData(json["event_data"].toObject());
        }

        // Set metadata
        QUuid userId = QUuid(userData["id"].toString());
        event->setCreatedBy(userId);
        event->setUpdatedBy(userId);

        bool success = m_repository->save(event);

        if (!success) {
            delete event;
            LOG_ERROR("Failed to create session event");
            return createErrorResponse("Failed to create session event", QHttpServerResponder::StatusCode::InternalServerError);
        }

        QJsonObject response = sessionEventToJson(event);
        LOG_INFO(QString("Session event created successfully for session %1: %2")
                .arg(sessionId).arg(event->id().toString()));
        delete event;

        return createSuccessResponse(response, QHttpServerResponder::StatusCode::Created);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception creating session event for session: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to create session event: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionEventController::handleUpdateEvent(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing UPDATE session event request: %1").arg(id));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid eventId = stringToUuid(QString::number(id));
        auto event = m_repository->getById(eventId);

        if (!event) {
            LOG_WARNING(QString("Session event not found with ID: %1").arg(id));
            return Http::Response::notFound("Session event not found");
        }

        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data");
            return createErrorResponse("Invalid JSON data", QHttpServerResponder::StatusCode::BadRequest);
        }

        // Update fields if provided
        if (json.contains("event_type") && !json["event_type"].toString().isEmpty()) {
            event->setEventType(stringToEventType(json["event_type"].toString()));
        }

        if (json.contains("event_time") && !json["event_time"].toString().isEmpty()) {
            event->setEventTime(QDateTime::fromString(json["event_time"].toString(), Qt::ISODate));
        }

        if (json.contains("user_id") && !json["user_id"].toString().isEmpty()) {
            event->setUserId(QUuid(json["user_id"].toString()));
        }

        if (json.contains("previous_user_id") && !json["previous_user_id"].toString().isEmpty()) {
            event->setPreviousUserId(QUuid(json["previous_user_id"].toString()));
        }

        if (json.contains("machine_id") && !json["machine_id"].toString().isEmpty()) {
            event->setMachineId(QUuid(json["machine_id"].toString()));
        }

        if (json.contains("terminal_session_id") && !json["terminal_session_id"].toString().isEmpty()) {
            event->setTerminalSessionId(json["terminal_session_id"].toString());
        }

        if (json.contains("is_remote")) {
            event->setIsRemote(json["is_remote"].toBool());
        }

        if (json.contains("event_data") && json["event_data"].isObject()) {
            event->setEventData(json["event_data"].toObject());
        }

        // Update metadata
        event->setUpdatedAt(QDateTime::currentDateTimeUtc());
        event->setUpdatedBy(QUuid(userData["id"].toString()));

        bool success = m_repository->update(event.data());

        if (!success) {
            LOG_ERROR(QString("Failed to update session event: %1").arg(id));
            return createErrorResponse("Failed to update session event", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Session event updated successfully: %1").arg(id));
        return createSuccessResponse(sessionEventToJson(event.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception updating session event: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to update session event: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionEventController::handleDeleteEvent(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing DELETE session event request: %1").arg(id));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid eventId = stringToUuid(QString::number(id));
        auto event = m_repository->getById(eventId);

        if (!event) {
            LOG_WARNING(QString("Session event not found with ID: %1").arg(id));
            return Http::Response::notFound("Session event not found");
        }

        bool success = m_repository->remove(eventId);

        if (!success) {
            LOG_ERROR(QString("Failed to delete session event: %1").arg(id));
            return createErrorResponse("Failed to delete session event", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Session event deleted successfully: %1").arg(id));
        // Return an empty response with NoContent status code (204)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception deleting session event: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to delete session event: %1").arg(e.what()));
    }
}

QHttpServerResponse SessionEventController::handleGetEventStats(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SessionEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET session event stats for session ID: %1").arg(sessionId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Create a statistics summary manually since getEventSummary doesn't exist
        QJsonObject summary;

        // Get all events for this session
        QList<QSharedPointer<SessionEventModel>> events = m_repository->getBySessionId(sessionUuid);

        // Count different event types
        int loginCount = 0;
        int logoutCount = 0;
        int lockCount = 0;
        int unlockCount = 0;
        int switchUserCount = 0;
        int remoteConnectCount = 0;
        int remoteDisconnectCount = 0;

        QDateTime firstEventTime;
        QDateTime lastEventTime;

        for (const auto &event : events) {
            // Update first and last event times
            if (firstEventTime.isNull() || event->eventTime() < firstEventTime) {
                firstEventTime = event->eventTime();
            }

            if (lastEventTime.isNull() || event->eventTime() > lastEventTime) {
                lastEventTime = event->eventTime();
            }

            // Count event types
            switch (event->eventType()) {
                case EventTypes::SessionEventType::Login:
                    loginCount++;
                    break;
                case EventTypes::SessionEventType::Logout:
                    logoutCount++;
                    break;
                case EventTypes::SessionEventType::Lock:
                    lockCount++;
                    break;
                case EventTypes::SessionEventType::Unlock:
                    unlockCount++;
                    break;
                case EventTypes::SessionEventType::SwitchUser:
                    switchUserCount++;
                    break;
                case EventTypes::SessionEventType::RemoteConnect:
                    remoteConnectCount++;
                    break;
                case EventTypes::SessionEventType::RemoteDisconnect:
                    remoteDisconnectCount++;
                    break;
            }
        }

        // Add statistics to summary
        summary["total_events"] = events.size();
        summary["login_count"] = loginCount;
        summary["logout_count"] = logoutCount;
        summary["lock_count"] = lockCount;
        summary["unlock_count"] = unlockCount;
        summary["switch_user_count"] = switchUserCount;
        summary["remote_connect_count"] = remoteConnectCount;
        summary["remote_disconnect_count"] = remoteDisconnectCount;

        if (!firstEventTime.isNull()) {
            summary["first_event_time"] = firstEventTime.toString(Qt::ISODate);
        }

        if (!lastEventTime.isNull()) {
            summary["last_event_time"] = lastEventTime.toString(Qt::ISODate);
        }

        if (!firstEventTime.isNull() && !lastEventTime.isNull()) {
            summary["duration_seconds"] = firstEventTime.secsTo(lastEventTime);
        }

        // Add session ID to the response
        summary["session_id"] = uuidToString(sessionUuid);

        if (events.isEmpty()) {
            LOG_WARNING(QString("No session event data found for session: %1").arg(sessionId));
            return createSuccessResponse(QJsonObject{{"message", "No session event data found for this session"}});
        }

        LOG_INFO(QString("Session event stats retrieved for session: %1").arg(sessionId));
        return createSuccessResponse(summary);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting session event stats: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve session event stats: %1").arg(e.what()));
    }
}

// Helper methods
QJsonObject SessionEventController::sessionEventToJson(SessionEventModel *event) const
{
    QJsonObject json;
    json["event_id"] = uuidToString(event->id());
    json["session_id"] = uuidToString(event->sessionId());
    json["event_type"] = eventTypeToString(event->eventType());
    json["event_time"] = event->eventTime().toString(Qt::ISODate);

    if (!event->userId().isNull()) {
        json["user_id"] = uuidToString(event->userId());
    }

    if (!event->previousUserId().isNull()) {
        json["previous_user_id"] = uuidToString(event->previousUserId());
    }

    json["machine_id"] = uuidToString(event->machineId());
    json["terminal_session_id"] = event->terminalSessionId();
    json["is_remote"] = event->isRemote();
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

QJsonObject SessionEventController::extractJsonFromRequest(const QHttpServerRequest &request, bool &ok)
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
    return doc.object();
}

QUuid SessionEventController::stringToUuid(const QString &str) const
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

QString SessionEventController::uuidToString(const QUuid &uuid) const
{
    return uuid.toString(QUuid::WithoutBraces);
}

EventTypes::SessionEventType SessionEventController::stringToEventType(const QString &eventTypeStr) const
{
    if (eventTypeStr == "login") {
        return EventTypes::SessionEventType::Login;
    } else if (eventTypeStr == "logout") {
        return EventTypes::SessionEventType::Logout;
    } else if (eventTypeStr == "lock") {
        return EventTypes::SessionEventType::Lock;
    } else if (eventTypeStr == "unlock") {
        return EventTypes::SessionEventType::Unlock;
    } else if (eventTypeStr == "switch_user") {
        return EventTypes::SessionEventType::SwitchUser;
    } else if (eventTypeStr == "remote_connect") {
        return EventTypes::SessionEventType::RemoteConnect;
    } else if (eventTypeStr == "remote_disconnect") {
        return EventTypes::SessionEventType::RemoteDisconnect;
    } else {
        LOG_WARNING(QString("Unknown event type string: %1, defaulting to Login").arg(eventTypeStr));
        return EventTypes::SessionEventType::Login;
    }
}

QString SessionEventController::eventTypeToString(EventTypes::SessionEventType eventType) const
{
    switch (eventType) {
    case EventTypes::SessionEventType::Login:
        return "login";
    case EventTypes::SessionEventType::Logout:
        return "logout";
    case EventTypes::SessionEventType::Lock:
        return "lock";
    case EventTypes::SessionEventType::Unlock:
        return "unlock";
    case EventTypes::SessionEventType::SwitchUser:
        return "switch_user";
    case EventTypes::SessionEventType::RemoteConnect:
        return "remote_connect";
    case EventTypes::SessionEventType::RemoteDisconnect:
        return "remote_disconnect";
    default:
        LOG_WARNING(QString("Unknown event type enum: %1, mapping to unknown").arg(static_cast<int>(eventType)));
        return "unknown";
    }
}

