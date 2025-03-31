#include "ActivityEventController.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QUrlQuery>
#include <Core/ModelFactory.h>

#include "logger/logger.h"
#include "httpserver/response.h"

// Constructor with default parameters
ActivityEventController::ActivityEventController(QObject *parent)
    : ApiControllerBase(parent)  // Changed from Http::Controller to ApiControllerBase
    , m_repository(nullptr)
    , m_authController(nullptr)
    , m_initialized(false)
{
    LOG_DEBUG("ActivityEventController created");
}

// Constructor with repository and auth controller
ActivityEventController::ActivityEventController(
    ActivityEventRepository* repository,
    AuthController* authController,
    QObject *parent)
    : ApiControllerBase(parent)  // Changed from Http::Controller to ApiControllerBase
    , m_repository(repository)
    , m_authController(authController)
    , m_initialized(false)
{
    LOG_DEBUG("ActivityEventController created with existing repository");

    // If a repository was provided, check if it's initialized
    if (m_repository && m_repository->isInitialized()) {
        m_initialized = true;
        LOG_INFO("ActivityEventController initialized successfully");
    }
}

// Destructor
ActivityEventController::~ActivityEventController()
{
    // Only delete repository if we created it
    if (m_repository && m_repository->parent() == this) {
        delete m_repository;
        m_repository = nullptr;
    }

    LOG_DEBUG("ActivityEventController destroyed");
}

bool ActivityEventController::initialize()
{
    if (m_initialized) {
        LOG_WARNING("ActivityEventController already initialized");
        return true;
    }

    LOG_DEBUG("Initializing ActivityEventController");

    try {
        // Check if repository is valid and initialized
        if (!m_repository) {
            LOG_ERROR("ActivityEvent repository not provided");
            return false;
        }

        if (!m_repository->isInitialized()) {
            LOG_ERROR("ActivityEvent repository not initialized");
            return false;
        }

        m_initialized = true;
        LOG_INFO("ActivityEventController initialized successfully");
        return true;
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception during ActivityEventController initialization: %1").arg(e.what()));
        return false;
    }
}

void ActivityEventController::setupRoutes(QHttpServer &server)
{
    if (!m_initialized) {
        LOG_ERROR("Cannot setup routes - ActivityEventController not initialized");
        return;
    }

    LOG_INFO("Setting up ActivityEventController routes");

    // Get all events
    server.route("/api/activities", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEvents(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get event by ID
    server.route("/api/activities/<arg>", QHttpServerRequest::Method::Get,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventById(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get events by session ID
    server.route("/api/sessions/<arg>/activities", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventsBySessionId(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get events by type for a session
    server.route("/api/sessions/<arg>/activities/type/<arg>", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QString &eventType, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventsByEventType(sessionId, eventType, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get events by time range for a session
    server.route("/api/sessions/<arg>/activities/timerange", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventsByTimeRange(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Create event
    server.route("/api/activities", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleCreateEvent(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Create event for session
    server.route("/api/sessions/<arg>/activities", QHttpServerRequest::Method::Post,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleCreateEventForSession(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Update event
    server.route("/api/activities/<arg>", QHttpServerRequest::Method::Put,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleUpdateEvent(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Delete event
    server.route("/api/activities/<arg>", QHttpServerRequest::Method::Delete,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleDeleteEvent(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get event statistics for session
    server.route("/api/sessions/<arg>/activities/stats", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetEventStats(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    LOG_INFO("ActivityEventController routes configured");
}

QHttpServerResponse ActivityEventController::handleGetEvents(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ActivityEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET all activity events request");

    // Check authentication - using base class method
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
        QList<QSharedPointer<ActivityEventModel>> events = m_repository->getAll();

        // Limit the results
        if (events.size() > limit) {
            events = events.mid(0, limit);
        }

        QJsonArray eventsArray;
        for (const auto &event : events) {
            eventsArray.append(activityEventToJson(event.data()));
        }

        LOG_INFO(QString("Retrieved %1 activity events").arg(events.size()));
        return createSuccessResponse(eventsArray);  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting activity events: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve activity events: %1").arg(e.what()));  // Using base class method
    }
}

QHttpServerResponse ActivityEventController::handleGetEventById(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ActivityEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET activity event by ID request: %1").arg(id));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid eventId = stringToUuid(QString::number(id));
        auto event = m_repository->getById(eventId);

        if (!event) {
            LOG_WARNING(QString("Activity event not found with ID: %1").arg(id));
            return Http::Response::notFound("Activity event not found");
        }

        LOG_INFO(QString("Retrieved activity event with ID: %1").arg(id));
        return createSuccessResponse(activityEventToJson(event.data()));  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting activity event by ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve activity event: %1").arg(e.what()));  // Using base class method
    }
}

QHttpServerResponse ActivityEventController::handleGetEventsBySessionId(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ActivityEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET activity events by session ID request: %1").arg(sessionId));

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
        int offset = query.queryItemValue("offset", QUrl::FullyDecoded).toInt();

        // Get activity events for the session
        QList<QSharedPointer<ActivityEventModel>> events = m_repository->getBySessionId(sessionUuid, limit, offset);

        QJsonArray eventsArray;
        for (const auto &event : events) {
            eventsArray.append(activityEventToJson(event.data()));
        }

        LOG_INFO(QString("Retrieved %1 activity events for session %2").arg(events.size()).arg(sessionId));
        return createSuccessResponse(eventsArray);  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting activity events by session ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve activity events: %1").arg(e.what()));  // Using base class method
    }
}

QHttpServerResponse ActivityEventController::handleGetEventsByEventType(const qint64 sessionId, const QString &eventType, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ActivityEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET activity events by event type: %1 for session: %2")
             .arg(eventType).arg(sessionId));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Convert string event type to enum
        EventTypes::ActivityEventType activityEventType = stringToEventType(eventType);

        // Parse query parameters
        QUrlQuery query(request.url().query());
        int limit = query.queryItemValue("limit", QUrl::FullyDecoded).toInt();
        int offset = query.queryItemValue("offset", QUrl::FullyDecoded).toInt();

        // Get activity events by type
        QList<QSharedPointer<ActivityEventModel>> events = m_repository->getByEventType(
            sessionUuid, activityEventType, limit, offset);

        QJsonArray eventsArray;
        for (const auto &event : events) {
            eventsArray.append(activityEventToJson(event.data()));
        }

        LOG_INFO(QString("Retrieved %1 activity events of type %2 for session %3")
                .arg(events.size()).arg(eventType).arg(sessionId));
        return createSuccessResponse(eventsArray);  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting activity events by event type: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve activity events: %1").arg(e.what()));  // Using base class method
    }
}

QHttpServerResponse ActivityEventController::handleGetEventsByTimeRange(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ActivityEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET activity events by time range for session: %1").arg(sessionId));

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
        QString startTimeStr = query.queryItemValue("start_time", QUrl::FullyDecoded);
        QString endTimeStr = query.queryItemValue("end_time", QUrl::FullyDecoded);
        int limit = query.queryItemValue("limit", QUrl::FullyDecoded).toInt();
        int offset = query.queryItemValue("offset", QUrl::FullyDecoded).toInt();

        if (startTimeStr.isEmpty() || endTimeStr.isEmpty()) {
            LOG_WARNING("Missing start_time or end_time parameters");
            return Http::Response::badRequest("Missing required parameters: start_time and end_time");
        }

        QDateTime startTime = QDateTime::fromString(startTimeStr, Qt::ISODate);
        QDateTime endTime = QDateTime::fromString(endTimeStr, Qt::ISODate);

        if (!startTime.isValid() || !endTime.isValid()) {
            LOG_WARNING("Invalid time format in parameters");
            return Http::Response::badRequest("Invalid time format. Use ISO format (YYYY-MM-DDThh:mm:ss)");
        }

        // Get activity events by time range
        QList<QSharedPointer<ActivityEventModel>> events = m_repository->getByTimeRange(
            sessionUuid, startTime, endTime, limit, offset);

        QJsonArray eventsArray;
        for (const auto &event : events) {
            eventsArray.append(activityEventToJson(event.data()));
        }

        LOG_INFO(QString("Retrieved %1 activity events in time range for session %2")
                .arg(events.size()).arg(sessionId));
        return createSuccessResponse(eventsArray);  // Using base class method
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting activity events by time range: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve activity events: %1").arg(e.what()));  // Using base class method
    }
}

// Handle creation of activity events
QHttpServerResponse ActivityEventController::handleCreateEvent(const QHttpServerRequest &request)
{
    // Check if controller is initialized
    if (!m_initialized) {
        LOG_ERROR("ActivityEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing CREATE activity event request");

    // Check authentication - using base class method
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
            return Http::Response::badRequest("Invalid JSON data");
        }

        // Validate required fields
        if (!json.contains("session_id") || json["session_id"].toString().isEmpty()) {
            LOG_WARNING("Missing required field: session_id");
            return Http::Response::badRequest("Session ID is required");
        }

        // Parse and validate session ID
        QUuid sessionUuid;
        try {
            sessionUuid = QUuid(json["session_id"].toString());
            if (sessionUuid.isNull()) {
                LOG_WARNING("Invalid session ID format");
                return Http::Response::badRequest("Invalid session ID format");
            }
        } catch (...) {
            LOG_WARNING("Exception parsing session ID");
            return Http::Response::badRequest("Invalid session ID format");
        }

        // Verify that the session exists using session repository
        bool sessionExists = false;
        if (m_sessionRepository && m_sessionRepository->isInitialized()) {
            auto session = m_sessionRepository->getById(sessionUuid);
            sessionExists = (session != nullptr);

            if (!sessionExists) {
                LOG_WARNING(QString("Session not found: %1").arg(sessionUuid.toString()));

                // Try to find an active session for this user/machine if provided
                if (json.contains("user_id") && json.contains("machine_id")) {
                    QUuid userId = QUuid(json["user_id"].toString());
                    QUuid machineId = QUuid(json["machine_id"].toString());

                    if (!userId.isNull() && !machineId.isNull()) {
                        LOG_INFO(QString("Trying to find active session for user %1 and machine %2")
                                .arg(userId.toString(), machineId.toString()));

                        auto activeSession = m_sessionRepository->getActiveSessionForUser(userId, machineId);

                        if (activeSession) {
                            LOG_INFO(QString("Found alternative active session: %1").arg(activeSession->id().toString()));
                            sessionUuid = activeSession->id();
                            sessionExists = true;
                        } else {
                            // If there's no active session, check if there's a session for today
                            QDate currentDate = QDateTime::currentDateTimeUtc().date();
                            auto todaySession = m_sessionRepository->getSessionForDay(userId, machineId, currentDate);

                            if (todaySession) {
                                LOG_INFO(QString("Found session for today: %1").arg(todaySession->id().toString()));
                                sessionUuid = todaySession->id();
                                sessionExists = true;
                            } else {
                                LOG_ERROR("No active session or today's session found for user/machine combination");
                                return Http::Response::badRequest("Session not found and no active session available");
                            }
                        }
                    } else {
                        return Http::Response::badRequest("Session not found and invalid user/machine IDs");
                    }
                } else {
                    return Http::Response::badRequest("Session not found");
                }
            }
        } else {
            LOG_WARNING("Session repository not available for validation");
            // For backward compatibility, continue even if we can't validate the session
            sessionExists = true;
        }

        // Create new activity event model
        ActivityEventModel *event = new ActivityEventModel();
        event->setSessionId(sessionUuid);

        // Handle app ID if provided
        if (json.contains("id") && !json["id"].toString().isEmpty()) {
            QUuid appId;
            try {
                appId = QUuid(json["id"].toString());
                if (!appId.isNull()) {
                    event->setAppId(appId);
                    LOG_DEBUG(QString("Setting app ID: %1").arg(appId.toString()));
                } else {
                    LOG_WARNING("Invalid app ID format, ignoring app_id field");
                }
            } catch (...) {
                LOG_WARNING("Exception parsing app ID, ignoring app_id field");
            }
        }

        // Set event type (with default if not provided)
        if (json.contains("event_type") && !json["event_type"].toString().isEmpty()) {
            EventTypes::ActivityEventType eventType = stringToEventType(json["event_type"].toString());
            event->setEventType(eventType);
            LOG_DEBUG(QString("Setting event type: %1").arg(eventTypeToString(eventType)));
        } else {
            // Default to MouseClick if not specified
            event->setEventType(EventTypes::ActivityEventType::MouseClick);
            LOG_DEBUG("No event type specified, defaulting to MouseClick");
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

        // Set event data if provided
        if (json.contains("event_data")) {
            if (json["event_data"].isObject()) {
                event->setEventData(json["event_data"].toObject());
                LOG_DEBUG("Event data JSON object set");
            } else {
                LOG_WARNING("event_data must be a JSON object, ignoring field");
            }
        }

        // Set metadata properties using ModelFactory
        ModelFactory::setCreationTimestamps(event, QUuid(userData["id"].toString()));

        // Save the event to the database
        LOG_DEBUG(QString("Attempting to save activity event: sessionId=%1, eventType=%2")
                 .arg(event->sessionId().toString(), eventTypeToString(event->eventType())));

        bool success = m_repository->save(event);

        if (!success) {
            delete event; // Clean up if save fails
            LOG_ERROR("Failed to create activity event: database operation failed");
            return createErrorResponse("Failed to create activity event",
                                      QHttpServerResponder::StatusCode::InternalServerError);
        }

        // Prepare success response
        QJsonObject response = activityEventToJson(event);

        // Add additional context to the response
        response["success"] = true;
        response["message"] = "Activity event created successfully";
        response["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        LOG_INFO(QString("Activity event created successfully: %1 (session: %2, type: %3)")
                .arg(event->id().toString(),
                     event->sessionId().toString(),
                     eventTypeToString(event->eventType())));

        delete event; // Clean up after successful operation

        // Return created response with 201 status code
        return createSuccessResponse(response, QHttpServerResponder::StatusCode::Created);
    }
    catch (const std::exception &e) {
        // Handle any unexpected exceptions
        LOG_ERROR(QString("Exception creating activity event: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to create activity event: %1").arg(e.what()),
                                   QHttpServerResponder::StatusCode::InternalServerError);
    }
}

QHttpServerResponse ActivityEventController::handleCreateEventForSession(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ActivityEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing CREATE activity event for session ID: %1").arg(sessionId));

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

        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Create new activity event
        ActivityEventModel *event = new ActivityEventModel();
        event->setSessionId(sessionUuid);

        // Set app ID if provided
        if (json.contains("id") && !json["id"].toString().isEmpty()) {
            event->setAppId(QUuid(json["id"].toString()));
        }

        // Set event type
        if (json.contains("event_type") && !json["event_type"].toString().isEmpty()) {
            event->setEventType(stringToEventType(json["event_type"].toString()));
        } else {
            // Default to MouseClick if not specified
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

        // Set metadata using ModelFactory
        ModelFactory::setCreationTimestamps(event, QUuid(userData["id"].toString()));

        bool success = m_repository->save(event);

        if (!success) {
            delete event;
            LOG_ERROR("Failed to create activity event");
            return createErrorResponse("Failed to create activity event",
                QHttpServerResponder::StatusCode::InternalServerError);
        }

        QJsonObject response = activityEventToJson(event);
        LOG_INFO(QString("Activity event created successfully for session %1: %2")
                .arg(sessionId).arg(event->id().toString()));
        delete event;

        return createSuccessResponse(response, QHttpServerResponder::StatusCode::Created);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception creating activity event for session: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to create activity event: %1").arg(e.what()));
    }
}

QHttpServerResponse ActivityEventController::handleUpdateEvent(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ActivityEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing UPDATE activity event request: %1").arg(id));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid eventId = stringToUuid(QString::number(id));
        auto event = m_repository->getById(eventId);

        if (!event) {
            LOG_WARNING(QString("Activity event not found with ID: %1").arg(id));
            return Http::Response::notFound("Activity event not found");
        }

        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data");
            return Http::Response::badRequest("Invalid JSON data");
        }

        // Update fields if provided
        if (json.contains("id") && !json["id"].toString().isEmpty()) {
            event->setAppId(QUuid(json["id"].toString()));
        }

        if (json.contains("event_type") && !json["event_type"].toString().isEmpty()) {
            event->setEventType(stringToEventType(json["event_type"].toString()));
        }

        if (json.contains("event_time") && !json["event_time"].toString().isEmpty()) {
            event->setEventTime(QDateTime::fromString(json["event_time"].toString(), Qt::ISODate));
        }

        if (json.contains("event_data") && json["event_data"].isObject()) {
            event->setEventData(json["event_data"].toObject());
        }

        // Update metadata using ModelFactory
        ModelFactory::setUpdateTimestamps(event.data(), QUuid(userData["id"].toString()));

        bool success = m_repository->update(event.data());

        if (!success) {
            LOG_ERROR(QString("Failed to update activity event: %1").arg(id));
            return createErrorResponse("Failed to update activity event",
                QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Activity event updated successfully: %1").arg(id));
        return createSuccessResponse(activityEventToJson(event.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception updating activity event: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to update activity event: %1").arg(e.what()));
    }
}

QHttpServerResponse ActivityEventController::handleDeleteEvent(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ActivityEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing DELETE activity event request: %1").arg(id));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid eventId = stringToUuid(QString::number(id));
        auto event = m_repository->getById(eventId);

        if (!event) {
            LOG_WARNING(QString("Activity event not found with ID: %1").arg(id));
            return Http::Response::notFound("Activity event not found");
        }

        bool success = m_repository->remove(eventId);

        if (!success) {
            LOG_ERROR(QString("Failed to delete activity event: %1").arg(id));
            return createErrorResponse("Failed to delete activity event",
                QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Activity event deleted successfully: %1").arg(id));
        // Return an empty response with NoContent status code (204)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception deleting activity event: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to delete activity event: %1").arg(e.what()));
    }
}

QHttpServerResponse ActivityEventController::handleGetEventStats(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ActivityEventController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET activity event stats for session ID: %1").arg(sessionId));

    // Check authentication - using base class method
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Get activity summary for the session
        QJsonObject summary = m_repository->getActivitySummary(sessionUuid);

        if (summary.isEmpty()) {
            LOG_WARNING(QString("No activity data found for session: %1").arg(sessionId));
            return createSuccessResponse(QJsonObject{{"message", "No activity data found for this session"}});
        }

        // Add session ID to the response
        summary["session_id"] = uuidToString(sessionUuid);

        LOG_INFO(QString("Activity event stats retrieved for session: %1").arg(sessionId));
        return createSuccessResponse(summary);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting activity event stats: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve activity event stats: %1").arg(e.what()));
    }
}

// Helper method: activityEventToJson - Creates a JSON object from an ActivityEventModel
QJsonObject ActivityEventController::activityEventToJson(ActivityEventModel *event) const
{
    QJsonObject json;
    json["event_id"] = uuidToString(event->id());
    json["session_id"] = uuidToString(event->sessionId());

    if (!event->appId().isNull()) {
        json["app_id"] = uuidToString(event->appId());
    }

    // Convert event type enum to string
    json["event_type"] = eventTypeToString(event->eventType());

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

// Helper method: stringToUuid - Converts a string to a QUuid
QUuid ActivityEventController::stringToUuid(const QString &str) const
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
QString ActivityEventController::uuidToString(const QUuid &uuid) const
{
    return uuid.toString(QUuid::WithoutBraces);
}

// Helper method: stringToEventType - Converts a string to an EventTypes::ActivityEventType
EventTypes::ActivityEventType ActivityEventController::stringToEventType(const QString &eventTypeStr)
{
    if (eventTypeStr == "mouse_click") {
        return EventTypes::ActivityEventType::MouseClick;
    } else if (eventTypeStr == "mouse_move") {
        return EventTypes::ActivityEventType::MouseMove;
    } else if (eventTypeStr == "keyboard") {
        return EventTypes::ActivityEventType::Keyboard;
    } else if (eventTypeStr == "afk_start") {
        return EventTypes::ActivityEventType::AfkStart;
    } else if (eventTypeStr == "afk_end") {
        return EventTypes::ActivityEventType::AfkEnd;
    } else if (eventTypeStr == "app_focus") {
        return EventTypes::ActivityEventType::AppFocus;
    } else if (eventTypeStr == "app_unfocus") {
        return EventTypes::ActivityEventType::AppUnfocus;
    } else {
        LOG_WARNING(QString("Unknown event type string: %1, defaulting to MouseClick").arg(eventTypeStr));
        return EventTypes::ActivityEventType::MouseClick;
    }
}

// Helper method: eventTypeToString - Converts an EventTypes::ActivityEventType to a string
QString ActivityEventController::eventTypeToString(EventTypes::ActivityEventType eventType) const
{
    switch (eventType) {
        case EventTypes::ActivityEventType::MouseClick:
            return "mouse_click";
        case EventTypes::ActivityEventType::MouseMove:
            return "mouse_move";
        case EventTypes::ActivityEventType::Keyboard:
            return "keyboard";
        case EventTypes::ActivityEventType::AfkStart:
            return "afk_start";
        case EventTypes::ActivityEventType::AfkEnd:
            return "afk_end";
        case EventTypes::ActivityEventType::AppFocus:
            return "app_focus";
        case EventTypes::ActivityEventType::AppUnfocus:
            return "app_unfocus";
        default:
            LOG_WARNING(QString("Unknown event type enum: %1, mapping to unknown").arg(static_cast<int>(eventType)));
            return "unknown";
    }
}

// Method to set the repository dependencies should be called in ApiServer initialization
void ActivityEventController::setRepositories(ActivityEventRepository* activityRepository, SessionRepository* sessionRepository) {
    m_repository = activityRepository;
    m_sessionRepository = sessionRepository;

    if (m_repository && m_repository->isInitialized() &&
        m_sessionRepository && m_sessionRepository->isInitialized()) {
        m_initialized = true;
        LOG_INFO("ActivityEventController initialized with all repositories");
        }
}