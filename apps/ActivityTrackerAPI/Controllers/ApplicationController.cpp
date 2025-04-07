#include "ApplicationController.h"
#include "Core/ModelFactory.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QUrlQuery>
#include <Core/ModelFactory.h>

#include "../Utils/SystemInfo.h"
#include "logger/logger.h"
#include "httpserver/response.h"

ApplicationController::ApplicationController(QObject *parent)
    : ApiControllerBase(parent)
    , m_applicationRepository(nullptr)
    , m_authController(nullptr)
    , m_initialized(false)
{
    LOG_DEBUG("ApplicationController created");
}

ApplicationController::ApplicationController(ApplicationRepository* repository, QObject *parent)
    : ApiControllerBase(parent)
    , m_applicationRepository(repository)
    , m_authController(nullptr)
    , m_initialized(false)
{
    LOG_DEBUG("ApplicationController created with existing repository");

    // If a repository was provided, check if it's initialized
    if (m_applicationRepository && m_applicationRepository->isInitialized()) {
        m_initialized = true;
        LOG_INFO("ApplicationController initialized successfully");
    }
}

ApplicationController::~ApplicationController()
{
    // Only delete repository if we created it
    if (m_applicationRepository && m_applicationRepository->parent() == this) {
        delete m_applicationRepository;
        m_applicationRepository = nullptr;
    }
    LOG_DEBUG("ApplicationController destroyed");
}

bool ApplicationController::initialize()
{
    if (m_initialized) {
        LOG_WARNING("ApplicationController already initialized");
        return true;
    }

    LOG_DEBUG("Initializing ApplicationController");

    try {
        // Check if repository is valid and initialized
        if (!m_applicationRepository) {
            LOG_ERROR("Application repository not provided");
            return false;
        }

        if (!m_applicationRepository->isInitialized()) {
            LOG_ERROR("Application repository not initialized");
            return false;
        }

        m_initialized = true;
        LOG_INFO("ApplicationController initialized successfully");
        return true;
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception during ApplicationController initialization: %1").arg(e.what()));
        return false;
    }
}

void ApplicationController::setupRoutes(QHttpServer &server)
{
    LOG_INFO("Setting up ApplicationController routes");

    // Get all applications
    server.route("/api/applications", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetApplications(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get application by ID
    server.route("/api/applications/<arg>", QHttpServerRequest::Method::Get,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetApplicationById(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Create a new application
    server.route("/api/applications", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleCreateApplication(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Update an application
    server.route("/api/applications/<arg>", QHttpServerRequest::Method::Put,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleUpdateApplication(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Delete an application
    server.route("/api/applications/<arg>", QHttpServerRequest::Method::Delete,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleDeleteApplication(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Assign application to role
    server.route("/api/applications/<arg>/roles/<arg>", QHttpServerRequest::Method::Post,
        [this](const qint64 appId, const qint64 roleId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleAssignApplicationToRole(appId, roleId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Remove application from role
    server.route("/api/applications/<arg>/roles/<arg>", QHttpServerRequest::Method::Delete,
        [this](const qint64 appId, const qint64 roleId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleRemoveApplicationFromRole(appId, roleId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Assign application to discipline
    server.route("/api/applications/<arg>/disciplines/<arg>", QHttpServerRequest::Method::Post,
        [this](const qint64 appId, const qint64 disciplineId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleAssignApplicationToDiscipline(appId, disciplineId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Remove application from discipline
    server.route("/api/applications/<arg>/disciplines/<arg>", QHttpServerRequest::Method::Delete,
        [this](const qint64 appId, const qint64 disciplineId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleRemoveApplicationFromDiscipline(appId, disciplineId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get restricted applications
    server.route("/api/applications/restricted", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetRestrictedApplications(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get tracked applications
    server.route("/api/applications/tracked", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetTrackedApplications(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Detect application
    server.route("/api/applications/detect", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleDetectApplication(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get applications by role
    server.route("/api/roles/<arg>/applications", QHttpServerRequest::Method::Get,
        [this](const qint64 roleId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetApplicationsByRole(roleId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get applications by discipline
    server.route("/api/disciplines/<arg>/applications", QHttpServerRequest::Method::Get,
        [this](const qint64 disciplineId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetApplicationsByDiscipline(disciplineId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    LOG_INFO("ApplicationController routes configured");
}

QHttpServerResponse ApplicationController::handleGetApplications(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET all applications request");

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        // Get all applications
        QList<QSharedPointer<ApplicationModel>> applications = m_applicationRepository->getAll();

        QJsonArray applicationsArray;
        for (const auto &app : applications) {
            applicationsArray.append(applicationToJson(app.data()));
        }

        return createSuccessResponse(applicationsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting applications: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve applications: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleGetApplicationById(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET application by ID request: %1").arg(id));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid appId = stringToUuid(QString::number(id));
        auto application = m_applicationRepository->getById(appId);

        if (!application) {
            LOG_WARNING(QString("Application not found with ID: %1").arg(id));
            return Http::Response::notFound("Application not found");
        }

        return createSuccessResponse(applicationToJson(application.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting application: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve application: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleCreateApplication(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing CREATE application request");

    // AuthController must be available
    if (!m_authController) {
        LOG_ERROR("AuthController not available");
        return createErrorResponse("Authentication service unavailable", QHttpServerResponder::StatusCode::ServiceUnavailable);
    }

    // Log the raw request body for debugging
    QString rawBody = QString::fromUtf8(request.body());
    LOG_DEBUG(QString("Raw request body: %1").arg(rawBody));

    // Check authentication - will automatically create user data if needed
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    // Log the user data obtained from authentication
    LOG_DEBUG(QString("User data after auth: %1").arg(QString::fromUtf8(QJsonDocument(userData).toJson())));

    try {
        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data");
            return Http::Response::badRequest("Invalid JSON data");
        }

        LOG_DEBUG(QString("Parsed JSON: %1").arg(QString::fromUtf8(QJsonDocument(json).toJson())));

        // Try to extract username from the JSON data
        QString username;

        // Check all possible source locations for a username
        if (json.contains("username") && !json["username"].toString().isEmpty()) {
            username = json["username"].toString();
            LOG_DEBUG(QString("Username found in JSON: %1").arg(username));
        } else if (userData.contains("username") && !userData["username"].toString().isEmpty()) {
            username = userData["username"].toString();
            LOG_DEBUG(QString("Username found in userData: %1").arg(username));
        } else if (userData.contains("name") && !userData["name"].toString().isEmpty()) {
            username = userData["name"].toString();
            LOG_DEBUG(QString("Using name as username: %1").arg(username));
        } else if (json.contains("name") && !json["name"].toString().isEmpty()) {
            username = json["name"].toString();
            LOG_DEBUG(QString("Using name field in JSON as username: %1").arg(username));
        } else if (json.contains("user") && json["user"].isObject()) {
            QJsonObject userObject = json["user"].toObject();
            if (userObject.contains("username") && !userObject["username"].toString().isEmpty()) {
                username = userObject["username"].toString();
                LOG_DEBUG(QString("Username found in JSON.user object: %1").arg(username));
            } else if (userObject.contains("name") && !userObject["name"].toString().isEmpty()) {
                username = userObject["name"].toString();
                LOG_DEBUG(QString("Using name from JSON.user object as username: %1").arg(username));
            }
        }

        // Final check - do we have a username?
        if (username.isEmpty()) {
            LOG_ERROR("No username could be identified from request or auth data");
            return Http::Response::badRequest("Unable to identify user for application creation");
        }

        LOG_INFO(QString("Proceeding with application creation for username: %1").arg(username));

        // Ensure the user exists in the database using AuthController
        auto user = m_authController->validateAndGetUserForTracking(username);

        if (!user) {
            LOG_ERROR(QString("Failed to ensure user exists: %1").arg(username));
            return createErrorResponse("Failed to create user account",
                QHttpServerResponder::StatusCode::InternalServerError);
        }

        // Extract required properties from the JSON
        QString appName;
        QString appPath;
        QString appHash;
        bool isRestricted = false;
        bool trackingEnabled = true;

        if (json.contains("app_name")) {
            appName = json["app_name"].toString();
        } else if (json.contains("name")) {
            appName = json["name"].toString();
        } else {
            LOG_ERROR("Application name is required");
            return Http::Response::badRequest("Application name is required");
        }

        // Set app path
        if (json.contains("app_path") && !json["app_path"].toString().isEmpty()) {
            appPath = json["app_path"].toString();
        } else {
            LOG_ERROR("App path is required");
            return Http::Response::badRequest("Application path is required");
        }

        // Set optional fields
        if (json.contains("app_hash")) {
            appHash = json["app_hash"].toString();
        }

        if (json.contains("is_restricted")) {
            isRestricted = json["is_restricted"].toBool();
        }

        if (json.contains("tracking_enabled")) {
            trackingEnabled = json["tracking_enabled"].toBool();
        }

        // Use findOrCreateApplication instead of manually creating and saving
        auto application = m_applicationRepository->findOrCreateApplication(
            appName,
            appPath,
            appHash,
            isRestricted,
            trackingEnabled,
            user->id()
        );

        if (!application) {
            LOG_ERROR("Failed to create or find application");
            return createErrorResponse("Failed to create or find application",
                QHttpServerResponder::StatusCode::InternalServerError);
        }

        QJsonObject response = applicationToJson(application.data());

        // Add a field to indicate whether this was newly created
        bool isNewlyCreated = (application->createdAt().secsTo(QDateTime::currentDateTimeUtc()) < 5);
        response["newly_created"] = isNewlyCreated;

        LOG_INFO(QString("Application %1 successfully: %2")
                .arg(isNewlyCreated ? "created" : "found")
                .arg(application->id().toString()));

        return createSuccessResponse(response,
            isNewlyCreated ? QHttpServerResponder::StatusCode::Created : QHttpServerResponder::StatusCode::Ok);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception creating application: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to create application: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleUpdateApplication(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing UPDATE application request: %1").arg(id));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid appId = stringToUuid(QString::number(id));
        auto application = m_applicationRepository->getById(appId);

        if (!application) {
            LOG_WARNING(QString("Application not found with ID: %1").arg(id));
            return Http::Response::notFound("Application not found");
        }

        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data");
            return Http::Response::badRequest("Invalid JSON data");
        }

        // Update fields if provided
        if (json.contains("app_name") && !json["app_name"].toString().isEmpty()) {
            application->setAppName(json["app_name"].toString());
        }

        if (json.contains("app_path") && !json["app_path"].toString().isEmpty()) {
            application->setAppPath(json["app_path"].toString());
        }

        if (json.contains("app_hash")) {
            application->setAppHash(json["app_hash"].toString());
        }

        if (json.contains("is_restricted")) {
            application->setIsRestricted(json["is_restricted"].toBool());
        }

        if (json.contains("tracking_enabled")) {
            application->setTrackingEnabled(json["tracking_enabled"].toBool());
        }

        // Update metadata using ModelFactory
        ModelFactory::setUpdateTimestamps(application.data(), QUuid(userData["id"].toString()));

        bool success = m_applicationRepository->update(application.data());

        if (!success) {
            LOG_ERROR(QString("Failed to update application: %1").arg(id));
            return createErrorResponse("Failed to update application", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Application updated successfully: %1").arg(id));
        return createSuccessResponse(applicationToJson(application.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception updating application: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to update application: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleDeleteApplication(const qint64 id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing DELETE application request: %1").arg(id));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid appId = stringToUuid(QString::number(id));
        auto application = m_applicationRepository->getById(appId);

        if (!application) {
            LOG_WARNING(QString("Application not found with ID: %1").arg(id));
            return Http::Response::notFound("Application not found");
        }

        bool success = m_applicationRepository->remove(appId);

        if (!success) {
            LOG_ERROR(QString("Failed to delete application: %1").arg(id));
            return createErrorResponse("Failed to delete application", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Application deleted successfully: %1").arg(id));
        // Return an empty response with NoContent status code (204)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception deleting application: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to delete application: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleAssignApplicationToRole(const qint64 appId, const qint64 roleId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing ASSIGN application %1 to role %2").arg(appId).arg(roleId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid appUuid = stringToUuid(QString::number(appId));
        QUuid roleUuid = stringToUuid(QString::number(roleId));
        QUuid userUuid = QUuid(userData["id"].toString());

        // Verify application exists
        auto application = m_applicationRepository->getById(appUuid);
        if (!application) {
            LOG_WARNING(QString("Application not found with ID: %1").arg(appId));
            return Http::Response::notFound("Application not found");
        }

        // Note: In a real implementation, we would also verify the role exists
        // but that would require access to a RoleRepository

        bool success = m_applicationRepository->assignApplicationToRole(appUuid, roleUuid, userUuid);

        if (!success) {
            LOG_ERROR(QString("Failed to assign application %1 to role %2").arg(appId).arg(roleId));
            return createErrorResponse("Failed to assign application to role", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Application %1 assigned to role %2 successfully").arg(appId).arg(roleId));

        // Return success response
        QJsonObject response;
        response["message"] = QString("Application successfully assigned to role");
        response["app_id"] = uuidToString(appUuid);
        response["role_id"] = uuidToString(roleUuid);

        return createSuccessResponse(response);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception assigning application to role: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to assign application to role: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleRemoveApplicationFromRole(const qint64 appId, const qint64 roleId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing REMOVE application %1 from role %2").arg(appId).arg(roleId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid appUuid = stringToUuid(QString::number(appId));
        QUuid roleUuid = stringToUuid(QString::number(roleId));

        // Verify application exists
        auto application = m_applicationRepository->getById(appUuid);
        if (!application) {
            LOG_WARNING(QString("Application not found with ID: %1").arg(appId));
            return Http::Response::notFound("Application not found");
        }

        bool success = m_applicationRepository->removeApplicationFromRole(appUuid, roleUuid);

        if (!success) {
            LOG_ERROR(QString("Failed to remove application %1 from role %2").arg(appId).arg(roleId));
            return createErrorResponse("Failed to remove application from role", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Application %1 removed from role %2 successfully").arg(appId).arg(roleId));
        // Return an empty response with NoContent status code (204)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception removing application from role: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to remove application from role: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleAssignApplicationToDiscipline(const qint64 appId, const qint64 disciplineId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing ASSIGN application %1 to discipline %2").arg(appId).arg(disciplineId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid appUuid = stringToUuid(QString::number(appId));
        QUuid disciplineUuid = stringToUuid(QString::number(disciplineId));
        QUuid userUuid = QUuid(userData["id"].toString());

        // Verify application exists
        auto application = m_applicationRepository->getById(appUuid);
        if (!application) {
            LOG_WARNING(QString("Application not found with ID: %1").arg(appId));
            return Http::Response::notFound("Application not found");
        }

        // Note: In a real implementation, we would also verify the discipline exists
        // but that would require access to a DisciplineRepository

        bool success = m_applicationRepository->assignApplicationToDiscipline(appUuid, disciplineUuid, userUuid);

        if (!success) {
            LOG_ERROR(QString("Failed to assign application %1 to discipline %2").arg(appId).arg(disciplineId));
            return createErrorResponse("Failed to assign application to discipline", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Application %1 assigned to discipline %2 successfully").arg(appId).arg(disciplineId));

        // Return success response
        QJsonObject response;
        response["message"] = QString("Application successfully assigned to discipline");
        response["app_id"] = uuidToString(appUuid);
        response["discipline_id"] = uuidToString(disciplineUuid);

        return createSuccessResponse(response);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception assigning application to discipline: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to assign application to discipline: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleRemoveApplicationFromDiscipline(const qint64 appId, const qint64 disciplineId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing REMOVE application %1 from discipline %2").arg(appId).arg(disciplineId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid appUuid = stringToUuid(QString::number(appId));
        QUuid disciplineUuid = stringToUuid(QString::number(disciplineId));

        // Verify application exists
        auto application = m_applicationRepository->getById(appUuid);
        if (!application) {
            LOG_WARNING(QString("Application not found with ID: %1").arg(appId));
            return Http::Response::notFound("Application not found");
        }

        bool success = m_applicationRepository->removeApplicationFromDiscipline(appUuid, disciplineUuid);

        if (!success) {
            LOG_ERROR(QString("Failed to remove application %1 from discipline %2").arg(appId).arg(disciplineId));
            return createErrorResponse("Failed to remove application from discipline", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Application %1 removed from discipline %2 successfully").arg(appId).arg(disciplineId));
        // Return an empty response with NoContent status code (204)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception removing application from discipline: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to remove application from discipline: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleGetRestrictedApplications(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET restricted applications request");

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QList<QSharedPointer<ApplicationModel>> applications = m_applicationRepository->getRestrictedApplications();

        QJsonArray applicationsArray;
        for (const auto &app : applications) {
            applicationsArray.append(applicationToJson(app.data()));
        }

        LOG_INFO(QString("Retrieved %1 restricted applications").arg(applications.size()));
        return createSuccessResponse(applicationsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting restricted applications: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve restricted applications: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleGetTrackedApplications(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET tracked applications request");

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QList<QSharedPointer<ApplicationModel>> applications = m_applicationRepository->getTrackedApplications();

        QJsonArray applicationsArray;
        for (const auto &app : applications) {
            applicationsArray.append(applicationToJson(app.data()));
        }

        LOG_INFO(QString("Retrieved %1 tracked applications").arg(applications.size()));
        return createSuccessResponse(applicationsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting tracked applications: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve tracked applications: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleDetectApplication(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing DETECT application request");

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
            return Http::Response::badRequest("Invalid JSON data");
        }

        // Extract required fields for application detection
        QString appName;
        QString appPath;
        QString appHash;
        bool isRestricted = false;
        bool trackingEnabled = true;

        if (json.contains("app_name") && !json["app_name"].toString().isEmpty()) {
            appName = json["app_name"].toString();
        } else {
            LOG_WARNING("App name is required for detection");
            return Http::Response::badRequest("App name is required for detection");
        }

        if (json.contains("app_path") && !json["app_path"].toString().isEmpty()) {
            appPath = json["app_path"].toString();
        } else {
            LOG_WARNING("App path is required for detection");
            return Http::Response::badRequest("App path is required for detection");
        }

        if (json.contains("app_hash")) {
            appHash = json["app_hash"].toString();
        }

        if (json.contains("is_restricted")) {
            isRestricted = json["is_restricted"].toBool();
        }

        if (json.contains("tracking_enabled")) {
            trackingEnabled = json["tracking_enabled"].toBool();
        }

        QUuid userId = QUuid(userData["id"].toString());

        // Try to find or create the application
        auto application = m_applicationRepository->findOrCreateApplication(
            appName, appPath, appHash, isRestricted, trackingEnabled, userId);

        if (!application) {
            LOG_ERROR("Failed to detect or create application");
            return createErrorResponse("Failed to detect or create application", QHttpServerResponder::StatusCode::InternalServerError);
        }

        // Return the application info with a boolean indicating if it was newly created
        QJsonObject response = applicationToJson(application.data());
        response["newly_created"] = (application->createdAt().secsTo(QDateTime::currentDateTimeUtc()) < 5); // Assume newly created if within last 5 seconds

        LOG_INFO(QString("Application detected successfully: %1").arg(application->id().toString()));
        return createSuccessResponse(response);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception detecting application: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to detect application: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleGetApplicationsByRole(const qint64 roleId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET applications by role: %1").arg(roleId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid roleUuid = stringToUuid(QString::number(roleId));

        QList<QSharedPointer<ApplicationModel>> applications = m_applicationRepository->getByRoleId(roleUuid);

        QJsonArray applicationsArray;
        for (const auto &app : applications) {
            applicationsArray.append(applicationToJson(app.data()));
        }

        LOG_INFO(QString("Retrieved %1 applications for role %2").arg(applications.size()).arg(roleId));
        return createSuccessResponse(applicationsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting applications by role: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve applications by role: %1").arg(e.what()));
    }
}

QHttpServerResponse ApplicationController::handleGetApplicationsByDiscipline(const qint64 disciplineId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("ApplicationController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET applications by discipline: %1").arg(disciplineId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid disciplineUuid = stringToUuid(QString::number(disciplineId));

        QList<QSharedPointer<ApplicationModel>> applications = m_applicationRepository->getByDisciplineId(disciplineUuid);

        QJsonArray applicationsArray;
        for (const auto &app : applications) {
            applicationsArray.append(applicationToJson(app.data()));
        }

        LOG_INFO(QString("Retrieved %1 applications for discipline %2").arg(applications.size()).arg(disciplineId));
        return createSuccessResponse(applicationsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting applications by discipline: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve applications by discipline: %1").arg(e.what()));
    }
}

QJsonObject ApplicationController::applicationToJson(ApplicationModel *application) const
{
    LOG_DEBUG(QString("Converting application to JSON: %1 (%2)")
            .arg(application->appName(), application->id().toString()));

    QJsonObject json;
    json["id"] = uuidToString(application->id());
    json["app_name"] = application->appName();
    json["app_path"] = application->appPath();
    json["app_hash"] = application->appHash();
    json["is_restricted"] = application->isRestricted();
    json["tracking_enabled"] = application->trackingEnabled();
    json["created_at"] = application->createdAt().toUTC().toString();

    if (!application->createdBy().isNull()) {
        json["created_by"] = uuidToString(application->createdBy());
    }

    json["updated_at"] = application->updatedAt().toUTC().toString();

    if (!application->updatedBy().isNull()) {
        json["updated_by"] = uuidToString(application->updatedBy());
    }

    LOG_DEBUG("Application converted to JSON successfully");
    return json;
}

QUuid ApplicationController::stringToUuid(const QString &str) const
{
    LOG_DEBUG(QString("Converting string to UUID: %1").arg(str));

    // Handle both simple format and UUID format
    if (str.contains('-')) {
        QUuid uuid = QUuid(str);
        LOG_DEBUG(QString("String converted to UUID with dashes: %1").arg(uuid.toString()));
        return uuid;
    } else {
        // Create UUID from simple format (without dashes)
        QString withDashes = str;
        withDashes.insert(8, '-');
        withDashes.insert(13, '-');
        withDashes.insert(18, '-');
        withDashes.insert(23, '-');

        QUuid uuid = QUuid(withDashes);
        LOG_DEBUG(QString("String converted to UUID with added dashes: %1").arg(uuid.toString()));
        return uuid;
    }
}

QString ApplicationController::uuidToString(const QUuid &uuid) const
{
    LOG_DEBUG(QString("Converting UUID to string: %1").arg(uuid.toString()));

    QString result = uuid.toString(QUuid::WithoutBraces);
    LOG_DEBUG(QString("UUID converted to string: %1").arg(result));

    return result;
}

void ApplicationController::setAuthController(std::shared_ptr<AuthController> authController)
{
    m_authController = authController;
    LOG_INFO("AuthController reference set in ApplicationController");
}

