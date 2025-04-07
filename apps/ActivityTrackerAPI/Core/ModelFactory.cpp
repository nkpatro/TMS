#include "ModelFactory.h"

// Include all model headers
#include "Models/UserModel.h"
#include "Models/TokenModel.h"
#include "Models/MachineModel.h"
#include "Models/SessionModel.h"
#include "Models/ActivityEventModel.h"
#include "Models/AfkPeriodModel.h"
#include "Models/ApplicationModel.h"
#include "Models/AppUsageModel.h"
#include "Models/DisciplineModel.h"
#include "Models/SystemMetricsModel.h"
#include "Models/RoleModel.h"
#include "Models/SessionEventModel.h"
#include "Models/UserRoleDisciplineModel.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QMetaObject>
#include <QMetaProperty>
#include <QDebug>
#include <QSqlRecord>
#include <QSysInfo>
#include "logger/logger.h"

//------------------------------------------------------------------------------
// Model creation from database query results
//------------------------------------------------------------------------------

QUuid ModelFactory::s_defaultCreatedBy;

UserModel* ModelFactory::createUserFromQuery(const QSqlQuery& query) {
    UserModel* user = new UserModel();

    user->setId(getUuidOrDefault(query, "id"));
    user->setName(getStringOrDefault(query, "name"));
    user->setEmail(getStringOrDefault(query, "email"));
    user->setPassword(getStringOrDefault(query, "password"));
    user->setPhoto(getStringOrDefault(query, "photo"));
    user->setActive(getBoolOrDefault(query, "active"));
    user->setVerified(getBoolOrDefault(query, "verified"));
    user->setVerificationCode(getStringOrDefault(query, "verification_code"));

    if (!query.value("status_id").isNull()) {
        user->setStatusId(QUuid(query.value("status_id").toString()));
    }

    setBaseModelFields(user, query);

    return user;
}

MachineModel* ModelFactory::createMachineFromQuery(const QSqlQuery& query) {
    MachineModel* machine = new MachineModel();

    machine->setId(getUuidOrDefault(query, "id"));
    machine->setName(getStringOrDefault(query, "name"));
    machine->setMachineUniqueId(getStringOrDefault(query, "machine_unique_id"));
    machine->setMacAddress(getStringOrDefault(query, "mac_address"));
    machine->setOperatingSystem(getStringOrDefault(query, "operating_system"));
    machine->setCpuInfo(getStringOrDefault(query, "cpu_info"));
    machine->setGpuInfo(getStringOrDefault(query, "gpu_info"));
    machine->setRamSizeGB(getIntOrDefault(query, "ram_size_gb"));
    machine->setIpAddress(getStringOrDefault(query, "ip_address"));
    machine->setLastSeenAt(getDateTimeOrDefault(query, "last_seen_at"));
    machine->setActive(getBoolOrDefault(query, "active"));

    setBaseModelFields(machine, query);

    return machine;
}

SessionModel* ModelFactory::createSessionFromQuery(const QSqlQuery& query) {
    SessionModel* session = new SessionModel();

    session->setId(getUuidOrDefault(query, "id"));
    session->setUserId(getUuidOrDefault(query, "user_id"));
    session->setMachineId(getUuidOrDefault(query, "machine_id"));

    session->setLoginTime(getDateTimeOrDefault(query, "login_time"));

    if (!query.value("logout_time").isNull()) {
        session->setLogoutTime(query.value("logout_time").toDateTime());
    }

    if (!query.value("session_data").isNull()) {
        QJsonDocument doc = QJsonDocument::fromJson(query.value("session_data").toByteArray());
        if (!doc.isNull() && doc.isObject()) {
            session->setSessionData(doc.object());
        }
    }

    if (!query.value("continued_from_session").isNull()) {
        session->setContinuedFromSession(QUuid(query.value("continued_from_session").toString()));
    }

    if (!query.value("continued_by_session").isNull()) {
        session->setContinuedBySession(QUuid(query.value("continued_by_session").toString()));
    }

    if (!query.value("previous_session_end_time").isNull()) {
        session->setPreviousSessionEndTime(query.value("previous_session_end_time").toDateTime());
    }

    session->setTimeSincePreviousSession(query.value("time_since_previous_session").toLongLong());

    setBaseModelFields(session, query);

    return session;
}

ActivityEventModel* ModelFactory::createActivityEventFromQuery(const QSqlQuery& query) {
    ActivityEventModel* event = new ActivityEventModel();

    event->setId(getUuidOrDefault(query, "id"));
    event->setSessionId(getUuidOrDefault(query, "session_id"));
    event->setEventType(static_cast<EventTypes::ActivityEventType>(getIntOrDefault(query, "event_type")));
    event->setEventTime(getDateTimeOrDefault(query, "event_time"));

    if (!query.value("event_data").isNull()) {
        QJsonDocument doc = QJsonDocument::fromJson(query.value("event_data").toByteArray());
        if (!doc.isNull() && doc.isObject()) {
            event->setEventData(doc.object());
        }
    }

    if (query.record().indexOf("app_id") != -1 && !query.value("app_id").isNull()) {
        event->setAppId(QUuid(query.value("app_id").toString()));
    }

    setBaseModelFields(event, query);
    return event;
}

AfkPeriodModel* ModelFactory::createAfkPeriodFromQuery(const QSqlQuery& query) {
    AfkPeriodModel* afkPeriod = new AfkPeriodModel();

    afkPeriod->setId(getUuidOrDefault(query, "id"));
    afkPeriod->setSessionId(getUuidOrDefault(query, "session_id"));
    afkPeriod->setStartTime(getDateTimeOrDefault(query, "start_time"));

    if (!query.value("end_time").isNull()) {
        afkPeriod->setEndTime(query.value("end_time").toDateTime());
    }

    setBaseModelFields(afkPeriod, query);

    return afkPeriod;
}

ApplicationModel* ModelFactory::createApplicationFromQuery(const QSqlQuery& query) {
    ApplicationModel* app = new ApplicationModel();

    app->setId(getUuidOrDefault(query, "id"));
    app->setAppName(getStringOrDefault(query, "app_name"));
    app->setAppPath(getStringOrDefault(query, "app_path"));
    app->setAppHash(getStringOrDefault(query, "app_hash"));
    app->setIsRestricted(getBoolOrDefault(query, "is_restricted"));
    app->setTrackingEnabled(getBoolOrDefault(query, "tracking_enabled"));

    setBaseModelFields(app, query);

    return app;
}

AppUsageModel* ModelFactory::createAppUsageFromQuery(const QSqlQuery& query) {
    AppUsageModel* usage = new AppUsageModel();

    usage->setId(getUuidOrDefault(query, "id"));
    usage->setSessionId(getUuidOrDefault(query, "session_id"));
    usage->setAppId(getUuidOrDefault(query, "app_id"));
    usage->setStartTime(getDateTimeOrDefault(query, "start_time"));

    if (!query.value("end_time").isNull()) {
        usage->setEndTime(query.value("end_time").toDateTime());
    }

    usage->setIsActive(getBoolOrDefault(query, "is_active"));
    usage->setWindowTitle(getStringOrDefault(query, "window_title"));

    setBaseModelFields(usage, query);

    return usage;
}

DisciplineModel* ModelFactory::createDisciplineFromQuery(const QSqlQuery& query) {
    DisciplineModel* discipline = new DisciplineModel();

    discipline->setId(getUuidOrDefault(query, "id"));
    discipline->setCode(getStringOrDefault(query, "code"));
    discipline->setName(getStringOrDefault(query, "name"));
    discipline->setDescription(getStringOrDefault(query, "description"));

    setBaseModelFields(discipline, query);

    return discipline;
}

SystemMetricsModel* ModelFactory::createSystemMetricsFromQuery(const QSqlQuery& query) {
    SystemMetricsModel* metrics = new SystemMetricsModel();

    metrics->setId(getUuidOrDefault(query, "id"));
    metrics->setSessionId(getUuidOrDefault(query, "session_id"));
    metrics->setCpuUsage(getDoubleOrDefault(query, "cpu_usage"));
    metrics->setMemoryUsage(getDoubleOrDefault(query, "memory_usage"));
    metrics->setGpuUsage(getDoubleOrDefault(query, "gpu_usage"));
    metrics->setMeasurementTime(getDateTimeOrDefault(query, "measurement_time"));

    setBaseModelFields(metrics, query);

    return metrics;
}

RoleModel* ModelFactory::createRoleFromQuery(const QSqlQuery& query) {
    RoleModel* role = new RoleModel();

    role->setId(getUuidOrDefault(query, "id"));
    role->setCode(getStringOrDefault(query, "code"));
    role->setName(getStringOrDefault(query, "name"));
    role->setDescription(getStringOrDefault(query, "description"));

    setBaseModelFields(role, query);

    return role;
}

SessionEventModel* ModelFactory::createSessionEventFromQuery(const QSqlQuery& query) {
    SessionEventModel* event = new SessionEventModel();

    event->setId(getUuidOrDefault(query, "id"));
    event->setSessionId(getUuidOrDefault(query, "session_id"));
    event->setEventType(static_cast<EventTypes::SessionEventType>(getIntOrDefault(query, "event_type")));
    event->setEventTime(getDateTimeOrDefault(query, "event_time"));
    event->setUserId(getUuidOrDefault(query, "user_id"));
    event->setPreviousUserId(getUuidOrDefault(query, "previous_user_id"));
    event->setMachineId(getUuidOrDefault(query, "machine_id"));
    event->setTerminalSessionId(getStringOrDefault(query, "terminal_session_id"));
    event->setIsRemote(getBoolOrDefault(query, "is_remote"));

    if (!query.value("event_data").isNull()) {
        QJsonDocument doc = QJsonDocument::fromJson(query.value("event_data").toByteArray());
        if (!doc.isNull() && doc.isObject()) {
            event->setEventData(doc.object());
        }
    }

    setBaseModelFields(event, query);

    return event;
}

UserRoleDisciplineModel* ModelFactory::createUserRoleDisciplineFromQuery(const QSqlQuery& query) {
    UserRoleDisciplineModel* urd = new UserRoleDisciplineModel();

    urd->setId(getUuidOrDefault(query, "id"));
    urd->setUserId(getUuidOrDefault(query, "user_id"));
    urd->setRoleId(getUuidOrDefault(query, "role_id"));
    urd->setDisciplineId(getUuidOrDefault(query, "discipline_id"));

    setBaseModelFields(urd, query);

    return urd;
}

//------------------------------------------------------------------------------
// Default model creation
//------------------------------------------------------------------------------

UserModel* ModelFactory::createDefaultUser(const QString& name, const QString& email) {
    UserModel* user = new UserModel();

    // Generate a unique ID
    user->setId(QUuid::createUuid());

    // Set provided values
    if (!name.isEmpty()) {
        user->setName(name);
    }

    if (!email.isEmpty()) {
        user->setEmail(email);
    }

    // Set defaults
    user->setActive(true);
    user->setVerified(false);

    // Set timestamps
    setCreationTimestamps(user);

    return user;
}

MachineModel* ModelFactory::createDefaultMachine(const QString& name) {
    MachineModel* machine = new MachineModel();

    // Generate a unique ID
    machine->setId(QUuid::createUuid());

    // Set name - use provided or default to hostname
    if (!name.isEmpty()) {
        machine->setName(name);
    } else {
        machine->setName(QSysInfo::machineHostName());
    }

    // Set defaults
    machine->setActive(true);
    machine->setLastSeenAt(QDateTime::currentDateTimeUtc());

    // Set timestamps
    setCreationTimestamps(machine);

    return machine;
}

SessionModel* ModelFactory::createDefaultSession(const QUuid& userId, const QUuid& machineId) {
    SessionModel* session = new SessionModel();

    // Generate a unique session ID
    session->setId(QUuid::createUuid());

    // Set required properties if provided
    if (!userId.isNull()) {
        session->setUserId(userId);
    }

    if (!machineId.isNull()) {
        session->setMachineId(machineId);
    }

    // Set default values
    session->setLoginTime(QDateTime::currentDateTimeUtc());
    session->setSessionData(QJsonObject());

    // Set timestamps
    setCreationTimestamps(session);

    return session;
}

ActivityEventModel* ModelFactory::createDefaultActivityEvent(const QUuid& sessionId) {
    ActivityEventModel* event = new ActivityEventModel();

    // Generate a unique ID
    event->setId(QUuid::createUuid());

    // Set session ID if provided
    if (!sessionId.isNull()) {
        event->setSessionId(sessionId);
    }

    // Set default values
    event->setEventTime(QDateTime::currentDateTimeUtc());
    event->setEventType(EventTypes::ActivityEventType::MouseClick); // Default
    event->setEventData(QJsonObject());

    // Set timestamps
    setCreationTimestamps(event);

    return event;
}

AfkPeriodModel* ModelFactory::createDefaultAfkPeriod(const QUuid& sessionId) {
    AfkPeriodModel* afkPeriod = new AfkPeriodModel();

    // Generate a unique ID
    afkPeriod->setId(QUuid::createUuid());

    // Set session ID if provided
    if (!sessionId.isNull()) {
        afkPeriod->setSessionId(sessionId);
    }

    // Set default values
    afkPeriod->setStartTime(QDateTime::currentDateTimeUtc());

    // Set timestamps
    setCreationTimestamps(afkPeriod);

    return afkPeriod;
}

ApplicationModel* ModelFactory::createDefaultApplication(const QString& name, const QString& appPath) {
    ApplicationModel* app = new ApplicationModel();

    // Generate a unique ID
    app->setId(QUuid::createUuid());

    // Set values if provided
    if (!name.isEmpty()) {
        app->setAppName(name);
    }

    if (!appPath.isEmpty()) {
        app->setAppPath(appPath);
    }

    // Set default values
    app->setTrackingEnabled(true);
    app->setIsRestricted(false);

    // Set timestamps
    setCreationTimestamps(app);

    return app;
}

AppUsageModel* ModelFactory::createDefaultAppUsage(const QUuid& sessionId, const QUuid& appId) {
    AppUsageModel* usage = new AppUsageModel();

    // Generate a unique ID
    usage->setId(QUuid::createUuid());

    // Set IDs if provided
    if (!sessionId.isNull()) {
        usage->setSessionId(sessionId);
    }

    if (!appId.isNull()) {
        usage->setAppId(appId);
    }

    // Set default values
    usage->setStartTime(QDateTime::currentDateTimeUtc());
    usage->setIsActive(true);

    // Set timestamps
    setCreationTimestamps(usage);

    return usage;
}

DisciplineModel* ModelFactory::createDefaultDiscipline(const QString& name) {
    DisciplineModel* discipline = new DisciplineModel();

    // Generate a unique ID
    discipline->setId(QUuid::createUuid());

    // Set name if provided
    if (!name.isEmpty()) {
        discipline->setName(name);
    }

    // Set timestamps
    setCreationTimestamps(discipline);

    return discipline;
}

SystemMetricsModel* ModelFactory::createDefaultSystemMetrics(const QUuid& sessionId) {
    SystemMetricsModel* metrics = new SystemMetricsModel();

    // Generate a unique ID
    metrics->setId(QUuid::createUuid());

    // Set session ID if provided
    if (!sessionId.isNull()) {
        metrics->setSessionId(sessionId);
    }

    // Set default values
    metrics->setMeasurementTime(QDateTime::currentDateTimeUtc());
    metrics->setCpuUsage(0.0);
    metrics->setMemoryUsage(0.0);
    metrics->setGpuUsage(0.0);

    // Set timestamps
    setCreationTimestamps(metrics);

    return metrics;
}

RoleModel* ModelFactory::createDefaultRole(const QString& name, const QString& code) {
    RoleModel* role = new RoleModel();

    // Generate a unique ID
    role->setId(QUuid::createUuid());

    // Set values if provided
    if (!name.isEmpty()) {
        role->setName(name);
    }

    if (!code.isEmpty()) {
        role->setCode(code);
    }

    // Set timestamps
    setCreationTimestamps(role);

    return role;
}

SessionEventModel* ModelFactory::createDefaultSessionEvent(const QUuid& sessionId) {
    SessionEventModel* event = new SessionEventModel();

    // Generate a unique ID
    event->setId(QUuid::createUuid());

    // Set session ID if provided
    if (!sessionId.isNull()) {
        event->setSessionId(sessionId);
    }

    // Set default values
    event->setEventTime(QDateTime::currentDateTimeUtc());
    event->setEventType(EventTypes::SessionEventType::Login); // Default
    event->setEventData(QJsonObject());
    event->setIsRemote(false);

    // Set timestamps
    setCreationTimestamps(event);

    return event;
}

UserRoleDisciplineModel* ModelFactory::createDefaultUserRoleDiscipline(const QUuid& userId, const QUuid& roleId) {
    UserRoleDisciplineModel* urd = new UserRoleDisciplineModel();

    // Generate a unique ID
    urd->setId(QUuid::createUuid());

    // Set IDs if provided
    if (!userId.isNull()) {
        urd->setUserId(userId);
    }

    if (!roleId.isNull()) {
        urd->setRoleId(roleId);
    }

    // Set timestamps
    setCreationTimestamps(urd);

    return urd;
}

//------------------------------------------------------------------------------
// Model validation
//------------------------------------------------------------------------------

bool ModelFactory::validateUserModel(const UserModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->name().isEmpty()) {
        errors.append("Name is required");
    }

    if (model->email().isEmpty()) {
        errors.append("Email is required");
    } else if (!model->email().contains('@')) {
        errors.append("Email must be a valid email address");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateMachineModel(const MachineModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->name().isEmpty()) {
        errors.append("Name is required");
    }

    if (model->machineUniqueId().isEmpty() && model->macAddress().isEmpty()) {
        errors.append("Either Machine Unique ID or MAC address is required");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateSessionModel(const SessionModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->userId().isNull()) {
        errors.append("User ID is required");
    }

    if (model->machineId().isNull()) {
        errors.append("Machine ID is required");
    }

    if (!model->loginTime().isValid()) {
        errors.append("Login time is required and must be valid");
    }

    // If logout time is set, it must be after login time
    if (model->logoutTime().isValid() && model->logoutTime() <= model->loginTime()) {
        errors.append("Logout time must be after login time");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateActivityEventModel(const ActivityEventModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->sessionId().isNull()) {
        errors.append("Session ID is required");
    }

    if (!model->eventTime().isValid()) {
        errors.append("Event time is required and must be valid");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateAfkPeriodModel(const AfkPeriodModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->sessionId().isNull()) {
        errors.append("Session ID is required");
    }

    if (!model->startTime().isValid()) {
        errors.append("Start time is required and must be valid");
    }

    // If end time is set, it must be after start time
    if (model->endTime().isValid() && model->endTime() <= model->startTime()) {
        errors.append("End time must be after start time");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateApplicationModel(const ApplicationModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->appName().isEmpty()) {
        errors.append("Application name is required");
    }

    if (model->appPath().isEmpty()) {
        errors.append("Application path is required");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateAppUsageModel(const AppUsageModel* model, QStringList& errors) {
    errors.clear();

    // if (model->id().isNull()) {
    //     errors.append("ID is required");
    // }

    if (model->sessionId().isNull()) {
        errors.append("Session ID is required");
    }

    if (model->appId().isNull()) {
        errors.append("App ID is required");
    }

    if (!model->startTime().isValid()) {
        errors.append("Start time is required and must be valid");
    }

    // If end time is set, it must be after start time
    if (model->endTime().isValid() && model->endTime() <= model->startTime()) {
        errors.append("End time must be after start time");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateDisciplineModel(const DisciplineModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->name().isEmpty()) {
        errors.append("Discipline name is required");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateSystemMetricsModel(const SystemMetricsModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->sessionId().isNull()) {
        errors.append("Session ID is required");
    }

    if (!model->measurementTime().isValid()) {
        errors.append("Measurement time is required and must be valid");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateRoleModel(const RoleModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->name().isEmpty()) {
        errors.append("Role name is required");
    }

    if (model->code().isEmpty()) {
        errors.append("Role code is required");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateSessionEventModel(const SessionEventModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->sessionId().isNull()) {
        errors.append("Session ID is required");
    }

    if (!model->eventTime().isValid()) {
        errors.append("Event time is required and must be valid");
    }

    return errors.isEmpty();
}

bool ModelFactory::validateUserRoleDisciplineModel(const UserRoleDisciplineModel* model, QStringList& errors) {
    errors.clear();

    if (model->id().isNull()) {
        errors.append("ID is required");
    }

    if (model->userId().isNull()) {
        errors.append("User ID is required");
    }

    if (model->roleId().isNull()) {
        errors.append("Role ID is required");
    }

    if (model->disciplineId().isNull()) {
        errors.append("Discipline ID is required");
    }

    return errors.isEmpty();
}

//------------------------------------------------------------------------------
// Timestamp management
//------------------------------------------------------------------------------

void ModelFactory::setCreationTimestamps(QObject* model, const QUuid& createdBy) {
    QDateTime now = QDateTime::currentDateTimeUtc();

    // Use introspection to set timestamps if the model has these properties
    const QMetaObject* metaObject = model->metaObject();

    // Set created_at
    int createdAtIndex = metaObject->indexOfProperty("createdAt");
    if (createdAtIndex != -1) {
        model->setProperty("createdAt", now);
    }

    // Set updated_at
    int updatedAtIndex = metaObject->indexOfProperty("updatedAt");
    if (updatedAtIndex != -1) {
        model->setProperty("updatedAt", now);
    }

    // Use provided user ID, or fall back to default
    QUuid userId = createdBy.isNull() ? s_defaultCreatedBy : createdBy;
    LOG_DEBUG(QString("Default Admin UserId: %1").arg(userId.toString()));

    // Set created_by if userId is provided
    if (!userId.isNull()) {
        int createdByIndex = metaObject->indexOfProperty("createdBy");
        if (createdByIndex != -1) {
            model->setProperty("createdBy", userId);
        }

        // Set updated_by
        int updatedByIndex = metaObject->indexOfProperty("updatedBy");
        if (updatedByIndex != -1) {
            model->setProperty("updatedBy", userId);
        }
    }
}

void ModelFactory::setUpdateTimestamps(QObject* model, const QUuid& updatedBy) {
    QDateTime now = QDateTime::currentDateTimeUtc();

    const QMetaObject* metaObject = model->metaObject();

    // Set updated_at
    int updatedAtIndex = metaObject->indexOfProperty("updatedAt");
    if (updatedAtIndex != -1) {
        model->setProperty("updatedAt", now);
    }

    // Use provided user ID, or fall back to default
    QUuid userId = updatedBy.isNull() ? s_defaultCreatedBy : updatedBy;

    // Set updated_by if userId is provided
    if (!userId.isNull()) {
        int updatedByIndex = metaObject->indexOfProperty("updatedBy");
        if (updatedByIndex != -1) {
            model->setProperty("updatedBy", userId);
        }
    }
}

//------------------------------------------------------------------------------
// JSON conversion utilities
//------------------------------------------------------------------------------

QJsonObject ModelFactory::modelToJson(const UserModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["name"] = model->name();
    json["email"] = model->email();
    json["photo"] = model->photo();
    json["active"] = model->active();
    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonObject ModelFactory::modelToJson(const SessionModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["user_id"] = model->userId().toString(QUuid::WithoutBraces);
    json["machine_id"] = model->machineId().toString(QUuid::WithoutBraces);
    json["login_time"] = model->loginTime().toUTC().toString();

    if (model->logoutTime().isValid()) {
        json["logout_time"] = model->logoutTime().toUTC().toString();
    }

    json["session_data"] = model->sessionData();

    if (!model->continuedFromSession().isNull()) {
        json["continued_from_session"] = model->continuedFromSession().toString(QUuid::WithoutBraces);
    }

    if (!model->continuedBySession().isNull()) {
        json["continued_by_session"] = model->continuedBySession().toString(QUuid::WithoutBraces);
    }

    if (model->previousSessionEndTime().isValid()) {
        json["previous_session_end_time"] = model->previousSessionEndTime().toUTC().toString();
    }

    json["time_since_previous_session"] = model->timeSincePreviousSession();
    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonObject ModelFactory::modelToJson(const ActivityEventModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["session_id"] = model->sessionId().toString(QUuid::WithoutBraces);
    json["event_type"] = static_cast<int>(model->eventType());
    json["event_time"] = model->eventTime().toUTC().toString();
    json["event_data"] = model->eventData();

    if (!model->appId().isNull()) {
        json["app_id"] = model->appId().toString(QUuid::WithoutBraces);
    }

    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonObject ModelFactory::modelToJson(const AfkPeriodModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["session_id"] = model->sessionId().toString(QUuid::WithoutBraces);
    json["start_time"] = model->startTime().toUTC().toString();

    if (model->endTime().isValid()) {
        json["end_time"] = model->endTime().toUTC().toString();
    }

    json["is_active"] = model->isActive();
    json["duration"] = model->duration();
    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonObject ModelFactory::modelToJson(const ApplicationModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["app_name"] = model->appName();
    json["app_path"] = model->appPath();
    json["app_hash"] = model->appHash();
    json["is_restricted"] = model->isRestricted();
    json["tracking_enabled"] = model->trackingEnabled();
    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonObject ModelFactory::modelToJson(const AppUsageModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["session_id"] = model->sessionId().toString(QUuid::WithoutBraces);
    json["app_id"] = model->appId().toString(QUuid::WithoutBraces);
    json["start_time"] = model->startTime().toUTC().toString();

    if (model->endTime().isValid()) {
        json["end_time"] = model->endTime().toUTC().toString();
    }

    json["is_active"] = model->isActive();
    json["window_title"] = model->windowTitle();
    json["duration"] = model->duration();
    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonObject ModelFactory::modelToJson(const DisciplineModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["code"] = model->code();
    json["name"] = model->name();
    json["description"] = model->description();
    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonObject ModelFactory::modelToJson(const SystemMetricsModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["session_id"] = model->sessionId().toString(QUuid::WithoutBraces);
    json["cpu_usage"] = model->cpuUsage();
    json["gpu_usage"] = model->gpuUsage();
    json["memory_usage"] = model->memoryUsage();
    json["measurement_time"] = model->measurementTime().toUTC().toString();
    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonObject ModelFactory::modelToJson(const RoleModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["code"] = model->code();
    json["name"] = model->name();
    json["description"] = model->description();
    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonObject ModelFactory::modelToJson(const SessionEventModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["session_id"] = model->sessionId().toString(QUuid::WithoutBraces);
    json["event_type"] = static_cast<int>(model->eventType());
    json["event_time"] = model->eventTime().toUTC().toString();
    json["user_id"] = model->userId().toString(QUuid::WithoutBraces);

    if (!model->previousUserId().isNull()) {
        json["previous_user_id"] = model->previousUserId().toString(QUuid::WithoutBraces);
    }

    json["machine_id"] = model->machineId().toString(QUuid::WithoutBraces);
    json["terminal_session_id"] = model->terminalSessionId();
    json["is_remote"] = model->isRemote();
    json["event_data"] = model->eventData();
    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonObject ModelFactory::modelToJson(const UserRoleDisciplineModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["user_id"] = model->userId().toString(QUuid::WithoutBraces);
    json["role_id"] = model->roleId().toString(QUuid::WithoutBraces);
    json["discipline_id"] = model->disciplineId().toString(QUuid::WithoutBraces);
    json["created_at"] = model->createdAt().toUTC().toString();
    json["updated_at"] = model->updatedAt().toUTC().toString();

    return json;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<UserModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<MachineModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<SessionModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<ActivityEventModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<AfkPeriodModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<ApplicationModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<AppUsageModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<DisciplineModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<SystemMetricsModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<RoleModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<SessionEventModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<UserRoleDisciplineModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

//------------------------------------------------------------------------------
// Helper methods for validation
//------------------------------------------------------------------------------

bool ModelFactory::validateRequiredFields(const QMap<QString, QVariant>& fields, QStringList& errors) {
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        if (it.value().isNull() || (it.value().type() == QVariant::String && it.value().toString().isEmpty())) {
            errors.append(QString("Field '%1' is required").arg(it.key()));
        }
    }

    return errors.isEmpty();
}

//------------------------------------------------------------------------------
// Helper methods for query value extraction
//------------------------------------------------------------------------------

template<typename T>
void ModelFactory::setBaseModelFields(T* model, const QSqlQuery& query) {
    // Set created_at field if present in query
    if (query.record().indexOf("created_at") != -1 && !query.value("created_at").isNull()) {
        model->setCreatedAt(query.value("created_at").toDateTime().toLocalTime());
    }

    // Set created_by field if present in query
    if (query.record().indexOf("created_by") != -1 && !query.value("created_by").isNull()) {
        model->setCreatedBy(QUuid(query.value("created_by").toString()));
    }

    // Set updated_at field if present in query
    if (query.record().indexOf("updated_at") != -1 && !query.value("updated_at").isNull()) {
        model->setUpdatedAt(query.value("updated_at").toDateTime().toLocalTime());
    }

    // Set updated_by field if present in query
    if (query.record().indexOf("updated_by") != -1 && !query.value("updated_by").isNull()) {
        model->setUpdatedBy(QUuid(query.value("updated_by").toString()));
    }
}

QUuid ModelFactory::getUuidOrDefault(const QSqlQuery& query, const QString& fieldName, const QUuid& defaultValue) {
    if (query.record().indexOf(fieldName) != -1 && !query.value(fieldName).isNull()) {
        QUuid uuid = QUuid(query.value(fieldName).toString());
        return uuid.isNull() ? defaultValue : uuid;
    }
    return defaultValue;
}

QString ModelFactory::getStringOrDefault(const QSqlQuery& query, const QString& fieldName, const QString& defaultValue) {
    if (query.record().indexOf(fieldName) != -1 && !query.value(fieldName).isNull()) {
        return query.value(fieldName).toString();
    }
    return defaultValue;
}

int ModelFactory::getIntOrDefault(const QSqlQuery& query, const QString& fieldName, int defaultValue) {
    if (query.record().indexOf(fieldName) != -1 && !query.value(fieldName).isNull()) {
        bool ok;
        int value = query.value(fieldName).toInt(&ok);
        return ok ? value : defaultValue;
    }
    return defaultValue;
}

double ModelFactory::getDoubleOrDefault(const QSqlQuery& query, const QString& fieldName, double defaultValue) {
    if (query.record().indexOf(fieldName) != -1 && !query.value(fieldName).isNull()) {
        bool ok;
        double value = query.value(fieldName).toDouble(&ok);
        return ok ? value : defaultValue;
    }
    return defaultValue;
}

bool ModelFactory::getBoolOrDefault(const QSqlQuery& query, const QString& fieldName, bool defaultValue) {
    if (query.record().indexOf(fieldName) != -1 && !query.value(fieldName).isNull()) {
        // Handle different ways to represent boolean in databases
        QVariant value = query.value(fieldName);
        if (value.type() == QVariant::Bool) {
            return value.toBool();
        } else if (value.type() == QVariant::Int) {
            return value.toInt() != 0;
        } else if (value.type() == QVariant::String) {
            QString str = value.toString().toLower();
            return str == "true" || str == "t" || str == "1" || str == "yes" || str == "y";
        }
    }
    return defaultValue;
}

QDateTime ModelFactory::getDateTimeOrDefault(const QSqlQuery& query, const QString& fieldName, const QDateTime& defaultValue) {
    if (query.record().indexOf(fieldName) != -1 && !query.value(fieldName).isNull()) {
        QDateTime dt = query.value(fieldName).toDateTime();
        return dt.isValid() ? dt : defaultValue;
    }
    return defaultValue;
}

QJsonObject ModelFactory::getJsonObjectOrDefault(const QSqlQuery& query, const QString& fieldName, const QJsonObject& defaultValue) {
    if (query.record().indexOf(fieldName) != -1 && !query.value(fieldName).isNull()) {
        QJsonDocument doc = QJsonDocument::fromJson(query.value(fieldName).toByteArray());
        if (doc.isObject()) {
            return doc.object();
        }
    }
    return defaultValue;
}

QJsonArray ModelFactory::getJsonArrayOrDefault(const QSqlQuery& query, const QString& fieldName, const QJsonArray& defaultValue) {
    if (query.record().indexOf(fieldName) != -1 && !query.value(fieldName).isNull()) {
        QJsonDocument doc = QJsonDocument::fromJson(query.value(fieldName).toByteArray());
        if (doc.isArray()) {
            return doc.array();
        }
    }
    return defaultValue;
}

QHostAddress ModelFactory::getHostAddressOrDefault(const QSqlQuery& query, const QString& fieldName, const QHostAddress& defaultValue) {
    if (query.record().indexOf(fieldName) != -1 && !query.value(fieldName).isNull()) {
        QString addressStr = query.value(fieldName).toString();
        if (!addressStr.isEmpty()) {
            QHostAddress address(addressStr);
            if (!address.isNull()) {
                return address;
            }
        }
    }
    return defaultValue;
}

QJsonObject ModelFactory::modelToJson(const MachineModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["name"] = model->name();
    json["machine_unique_id"] = model->machineUniqueId();
    json["mac_address"] = model->macAddress();
    json["operating_system"] = model->operatingSystem();
    json["cpu_info"] = model->cpuInfo();
    json["gpu_info"] = model->gpuInfo();
    json["ram_size_gb"] = model->ramSizeGB();
    json["ip_address"] = model->ipAddress();

    if (model->lastSeenAt().isValid()) {
        json["last_seen_at"] = model->lastSeenAt().toUTC().toString();
    }

    json["active"] = model->active();
    return json;
}

void ModelFactory::setDefaultCreatedBy(const QUuid& userId) {
    LOG_DEBUG(QString("Setting default created_by user ID: %1").arg(userId.toString()));
    s_defaultCreatedBy = userId;
}

QUuid ModelFactory::getDefaultCreatedBy() {
    return s_defaultCreatedBy;
}

TokenModel* ModelFactory::createTokenFromQuery(const QSqlQuery& query) {
    TokenModel* token = new TokenModel();

    // Set primary UUID if available
    token->setId(getUuidOrDefault(query, "id"));

    token->setTokenId(getStringOrDefault(query, "token_id"));
    token->setTokenType(getStringOrDefault(query, "token_type"));
    token->setUserId(getUuidOrDefault(query, "user_id"));

    // Handle JSON fields
    token->setTokenData(getJsonObjectOrDefault(query, "token_data"));
    token->setDeviceInfo(getJsonObjectOrDefault(query, "device_info"));

    // Handle date fields
    token->setExpiresAt(getDateTimeOrDefault(query, "expires_at"));
    token->setLastUsedAt(getDateTimeOrDefault(query, "last_used_at"));

    // Handle boolean and other fields
    token->setRevoked(getBoolOrDefault(query, "revoked"));
    token->setRevocationReason(getStringOrDefault(query, "revocation_reason"));

    // Set base model fields (created_at, created_by, etc.)
    setBaseModelFields(token, query);

    return token;
}

TokenModel* ModelFactory::createDefaultToken(const QString& tokenId, const QUuid& userId, const QString& tokenType) {
    TokenModel* token = new TokenModel();

    // Generate a unique ID
    token->setId(QUuid::createUuid());

    // Set provided values
    if (!tokenId.isEmpty()) {
        token->setTokenId(tokenId);
    } else {
        // Generate a random token if none provided
        token->setTokenId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    }

    if (!userId.isNull()) {
        token->setUserId(userId);
    }

    if (!tokenType.isEmpty()) {
        token->setTokenType(tokenType);
    }

    // Default expirations vary by token type
    QDateTime now = QDateTime::currentDateTimeUtc();
    if (tokenType == "refresh") {
        // Refresh tokens typically last longer (30 days)
        token->setExpiresAt(now.addDays(30));
    } else if (tokenType == "api") {
        // API keys typically last even longer (1 year)
        token->setExpiresAt(now.addDays(365));
    } else {
        // Standard user token (24 hours)
        token->setExpiresAt(now.addDays(1));
    }

    // Set empty JSON objects for token data and device info
    token->setTokenData(QJsonObject());
    token->setDeviceInfo(QJsonObject());

    // Set revocation status
    token->setRevoked(false);
    token->setRevocationReason(QString());

    // Set timestamps
    setCreationTimestamps(token);
    token->setLastUsedAt(now);

    return token;
}

bool ModelFactory::validateTokenModel(const TokenModel* model, QStringList& errors) {
    errors.clear();

    if (model->tokenId().isEmpty()) {
        errors.append("Token ID is required");
    }

    if (model->tokenType().isEmpty()) {
        errors.append("Token type is required");
    }

    if (model->userId().isNull()) {
        errors.append("User ID is required");
    }

    if (!model->expiresAt().isValid()) {
        errors.append("Expiration time is required and must be valid");
    }

    // Validate that expiration is in the future
    if (model->expiresAt() <= QDateTime::currentDateTimeUtc()) {
        errors.append("Expiration time must be in the future");
    }

    return errors.isEmpty();
}

QJsonObject ModelFactory::modelToJson(const TokenModel* model) {
    if (!model) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = model->id().toString(QUuid::WithoutBraces);
    json["token_id"] = model->tokenId();
    json["token_type"] = model->tokenType();
    json["user_id"] = model->userId().toString(QUuid::WithoutBraces);
    json["expires_at"] = model->expiresAt().toUTC().toString();
    json["created_at"] = model->createdAt().toUTC().toString();

    if (!model->createdBy().isNull()) {
        json["created_by"] = model->createdBy().toString(QUuid::WithoutBraces);
    }

    json["updated_at"] = model->updatedAt().toUTC().toString();

    if (!model->updatedBy().isNull()) {
        json["updated_by"] = model->updatedBy().toString(QUuid::WithoutBraces);
    }

    json["revoked"] = model->isRevoked();

    if (!model->revocationReason().isEmpty()) {
        json["revocation_reason"] = model->revocationReason();
    }

    json["last_used_at"] = model->lastUsedAt().toUTC().toString();

    // Include token data and device info
    json["token_data"] = model->tokenData();
    json["device_info"] = model->deviceInfo();

    // Include helper fields for token status
    json["is_expired"] = model->isExpired();
    json["is_valid"] = model->isValid();

    return json;
}

QJsonArray ModelFactory::modelsToJsonArray(const QList<QSharedPointer<TokenModel>>& models) {
    QJsonArray array;
    for (const auto& model : models) {
        array.append(modelToJson(model.data()));
    }
    return array;
}

