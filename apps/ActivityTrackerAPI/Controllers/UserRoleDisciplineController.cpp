#include "UserRoleDisciplineController.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrlQuery>
#include "logger/logger.h"
#include "httpserver/response.h"

UserRoleDisciplineController::UserRoleDisciplineController(UserRoleDisciplineRepository *repository, QObject *parent)
    : ApiControllerBase(parent), m_repository(repository)
{
    LOG_INFO("UserRoleDisciplineController created");
}

UserRoleDisciplineController::UserRoleDisciplineController(UserRoleDisciplineRepository *repository,
                                                       AuthController *authController,
                                                       QObject *parent)
    : ApiControllerBase(parent), m_repository(repository), m_authController(authController)
{
    LOG_INFO("UserRoleDisciplineController created with auth controller");
}

void UserRoleDisciplineController::setupRoutes(QHttpServer &server)
{
    LOG_INFO("Setting up UserRoleDisciplineController routes");

    // Get all assignments
    server.route("/api/user-role-disciplines", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetAllAssignments(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get assignments for a specific user
    server.route("/api/users/<arg>/role-disciplines", QHttpServerRequest::Method::Get,
        [this](const qint64 userId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetUserAssignments(userId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get assignments for a specific role
    server.route("/api/roles/<arg>/user-disciplines", QHttpServerRequest::Method::Get,
        [this](const qint64 roleId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetRoleAssignments(roleId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get assignments for a specific discipline
    server.route("/api/disciplines/<arg>/user-roles", QHttpServerRequest::Method::Get,
        [this](const qint64 disciplineId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetDisciplineAssignments(disciplineId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Create a new assignment
    server.route("/api/user-role-disciplines", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleAssignRoleDiscipline(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Update an assignment
    server.route("/api/user-role-disciplines/<arg>", QHttpServerRequest::Method::Put,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleUpdateAssignment(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Delete an assignment
    server.route("/api/user-role-disciplines/<arg>", QHttpServerRequest::Method::Delete,
        [this](const qint64 id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleRemoveAssignment(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Check if a user has a role in a discipline
    server.route("/api/user-role-disciplines/check", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleCheckAssignment(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    LOG_INFO("UserRoleDisciplineController routes set up successfully");
}

QHttpServerResponse UserRoleDisciplineController::handleGetAllAssignments(const QHttpServerRequest &request)
{
    LOG_DEBUG("Processing GET all user-role-discipline assignments");

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QList<QSharedPointer<UserRoleDisciplineModel>> assignments = m_repository->getAll();

        QJsonArray assignmentsArray;
        for (const auto &assignment : assignments) {
            assignmentsArray.append(assignmentToJson(assignment.data()));
        }

        LOG_INFO(QString("Retrieved %1 user-role-discipline assignments").arg(assignments.size()));
        return createSuccessResponse(assignmentsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting assignments: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve assignments: %1").arg(e.what()));
    }
}

QHttpServerResponse UserRoleDisciplineController::handleGetUserAssignments(const qint64 userId, const QHttpServerRequest &request)
{
    LOG_DEBUG(QString("Processing GET assignments for user: %1").arg(userId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid userUuid = stringToUuid(QString::number(userId));
        QList<QSharedPointer<UserRoleDisciplineModel>> assignments = m_repository->getByUserId(userUuid);

        QJsonArray assignmentsArray;
        for (const auto &assignment : assignments) {
            assignmentsArray.append(assignmentToJson(assignment.data()));
        }

        LOG_INFO(QString("Retrieved %1 assignments for user %2").arg(assignments.size()).arg(userId));
        return createSuccessResponse(assignmentsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting user assignments: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve user assignments: %1").arg(e.what()));
    }
}

QHttpServerResponse UserRoleDisciplineController::handleGetRoleAssignments(const qint64 roleId, const QHttpServerRequest &request)
{
    LOG_DEBUG(QString("Processing GET assignments for role: %1").arg(roleId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid roleUuid = stringToUuid(QString::number(roleId));
        QList<QSharedPointer<UserRoleDisciplineModel>> assignments = m_repository->getByRoleId(roleUuid);

        QJsonArray assignmentsArray;
        for (const auto &assignment : assignments) {
            assignmentsArray.append(assignmentToJson(assignment.data()));
        }

        LOG_INFO(QString("Retrieved %1 assignments for role %2").arg(assignments.size()).arg(roleId));
        return createSuccessResponse(assignmentsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting role assignments: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve role assignments: %1").arg(e.what()));
    }
}

QHttpServerResponse UserRoleDisciplineController::handleGetDisciplineAssignments(const qint64 disciplineId, const QHttpServerRequest &request)
{
    LOG_DEBUG(QString("Processing GET assignments for discipline: %1").arg(disciplineId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid disciplineUuid = stringToUuid(QString::number(disciplineId));
        QList<QSharedPointer<UserRoleDisciplineModel>> assignments = m_repository->getByDisciplineId(disciplineUuid);

        QJsonArray assignmentsArray;
        for (const auto &assignment : assignments) {
            assignmentsArray.append(assignmentToJson(assignment.data()));
        }

        LOG_INFO(QString("Retrieved %1 assignments for discipline %2").arg(assignments.size()).arg(disciplineId));
        return createSuccessResponse(assignmentsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting discipline assignments: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve discipline assignments: %1").arg(e.what()));
    }
}

QHttpServerResponse UserRoleDisciplineController::handleAssignRoleDiscipline(const QHttpServerRequest &request)
{
    LOG_DEBUG("Processing CREATE user-role-discipline assignment");

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

        // Validate required fields
        if (!json.contains("user_id") || !json.contains("role_id") || !json.contains("discipline_id")) {
            LOG_WARNING("Missing required fields in JSON");
            return createErrorResponse("user_id, role_id, and discipline_id are required", QHttpServerResponder::StatusCode::BadRequest);
        }

        // Create new model
        UserRoleDisciplineModel *model = new UserRoleDisciplineModel();
        model->setUserId(QUuid(json["user_id"].toString()));
        model->setRoleId(QUuid(json["role_id"].toString()));
        model->setDisciplineId(QUuid(json["discipline_id"].toString()));

        // Set metadata
        QUuid creatorId = QUuid(userData["id"].toString());
        model->setCreatedBy(creatorId);
        model->setUpdatedBy(creatorId);

        // Check if assignment already exists
        if (m_repository->userHasRoleInDiscipline(model->userId(), model->roleId(), model->disciplineId())) {
            delete model;
            LOG_WARNING("Assignment already exists");
            return createErrorResponse("User already has this role in this discipline", QHttpServerResponder::StatusCode::Conflict);
        }

        bool success = m_repository->save(model);

        if (!success) {
            delete model;
            LOG_ERROR("Failed to create assignment");
            return createErrorResponse("Failed to create assignment", QHttpServerResponder::StatusCode::InternalServerError);
        }

        QJsonObject response = assignmentToJson(model);
        LOG_INFO(QString("Assignment created successfully: %1").arg(model->id().toString()));
        delete model;

        return createSuccessResponse(response, QHttpServerResponder::StatusCode::Created);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception creating assignment: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to create assignment: %1").arg(e.what()));
    }
}

QHttpServerResponse UserRoleDisciplineController::handleUpdateAssignment(const qint64 id, const QHttpServerRequest &request)
{
    LOG_DEBUG(QString("Processing UPDATE assignment: %1").arg(id));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid assignmentId = stringToUuid(QString::number(id));
        auto existingAssignment = m_repository->getById(assignmentId);

        if (!existingAssignment) {
            LOG_WARNING(QString("Assignment not found with ID: %1").arg(id));
            return Http::Response::notFound("Assignment not found");
        }

        bool ok;
        QJsonObject json = extractJsonFromRequest(request, ok);
        if (!ok) {
            LOG_WARNING("Invalid JSON data");
            return createErrorResponse("Invalid JSON data", QHttpServerResponder::StatusCode::BadRequest);
        }

        // Update fields if provided in JSON
        if (json.contains("user_id")) {
            existingAssignment->setUserId(QUuid(json["user_id"].toString()));
        }

        if (json.contains("role_id")) {
            existingAssignment->setRoleId(QUuid(json["role_id"].toString()));
        }

        if (json.contains("discipline_id")) {
            existingAssignment->setDisciplineId(QUuid(json["discipline_id"].toString()));
        }

        // Update metadata
        existingAssignment->setUpdatedAt(QDateTime::currentDateTimeUtc());
        existingAssignment->setUpdatedBy(QUuid(userData["id"].toString()));

        bool success = m_repository->update(existingAssignment.data());

        if (!success) {
            LOG_ERROR(QString("Failed to update assignment: %1").arg(id));
            return createErrorResponse("Failed to update assignment", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Assignment updated successfully: %1").arg(id));
        return createSuccessResponse(assignmentToJson(existingAssignment.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception updating assignment: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to update assignment: %1").arg(e.what()));
    }
}

QHttpServerResponse UserRoleDisciplineController::handleRemoveAssignment(const qint64 id, const QHttpServerRequest &request)
{
    LOG_DEBUG(QString("Processing DELETE assignment: %1").arg(id));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid assignmentId = stringToUuid(QString::number(id));
        bool success = m_repository->remove(assignmentId);

        if (!success) {
            LOG_ERROR(QString("Failed to delete assignment: %1").arg(id));
            return createErrorResponse("Failed to delete assignment", QHttpServerResponder::StatusCode::InternalServerError);
        }

        LOG_INFO(QString("Assignment deleted successfully: %1").arg(id));
        return createSuccessResponse(QJsonObject{{"success", true}});
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception deleting assignment: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to delete assignment: %1").arg(e.what()));
    }
}

QHttpServerResponse UserRoleDisciplineController::handleCheckAssignment(const QHttpServerRequest &request)
{
    LOG_DEBUG("Processing CHECK user-role-discipline assignment");

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

        // Validate required fields
        if (!json.contains("user_id") || !json.contains("role_id") || !json.contains("discipline_id")) {
            LOG_WARNING("Missing required fields in JSON");
            return createErrorResponse("user_id, role_id, and discipline_id are required", QHttpServerResponder::StatusCode::BadRequest);
        }

        QUuid userId = QUuid(json["user_id"].toString());
        QUuid roleId = QUuid(json["role_id"].toString());
        QUuid disciplineId = QUuid(json["discipline_id"].toString());

        bool hasAssignment = m_repository->userHasRoleInDiscipline(userId, roleId, disciplineId);

        QJsonObject response{
            {"user_id", userId.toString(QUuid::WithoutBraces)},
            {"role_id", roleId.toString(QUuid::WithoutBraces)},
            {"discipline_id", disciplineId.toString(QUuid::WithoutBraces)},
            {"has_assignment", hasAssignment}
        };

        LOG_INFO(QString("Assignment check completed for user: %1, role: %2, discipline: %3, result: %4")
                 .arg(userId.toString(), roleId.toString(), disciplineId.toString())
                 .arg(hasAssignment ? "true" : "false"));

        return createSuccessResponse(response);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception checking assignment: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to check assignment: %1").arg(e.what()));
    }
}

QJsonObject UserRoleDisciplineController::assignmentToJson(UserRoleDisciplineModel *model) const
{
    QJsonObject json;
    json["id"] = uuidToString(model->id());
    json["user_id"] = uuidToString(model->userId());
    json["role_id"] = uuidToString(model->roleId());
    json["discipline_id"] = uuidToString(model->disciplineId());
    json["created_at"] = model->createdAt().toUTC().toString();

    if (!model->createdBy().isNull()) {
        json["created_by"] = uuidToString(model->createdBy());
    }

    json["updated_at"] = model->updatedAt().toUTC().toString();

    if (!model->updatedBy().isNull()) {
        json["updated_by"] = uuidToString(model->updatedBy());
    }

    return json;
}

QJsonObject UserRoleDisciplineController::extractJsonFromRequest(const QHttpServerRequest &request, bool &ok)
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

QUuid UserRoleDisciplineController::stringToUuid(const QString &str) const
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

QString UserRoleDisciplineController::uuidToString(const QUuid &uuid) const
{
    return uuid.toString(QUuid::WithoutBraces);
}

