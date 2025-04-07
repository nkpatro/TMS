#include "SessionEventRepository.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include "Core/ModelFactory.h"

SessionEventRepository::SessionEventRepository(QObject *parent)
    : BaseRepository<SessionEventModel>(parent)
{
    LOG_DEBUG("SessionEventRepository created");
}

QString SessionEventRepository::getEntityName() const
{
    return "SessionEvent";
}

QString SessionEventRepository::getModelId(SessionEventModel* model) const
{
    return model->id().toString();
}

QString SessionEventRepository::buildSaveQuery()
{
    return "INSERT INTO session_events "
           "(session_id, event_type, event_time, user_id, previous_user_id, "
           "machine_id, terminal_session_id, is_remote, event_data, "
           "created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:session_id, :event_type, :event_time, :user_id, "
           ":previous_user_id, :machine_id, :terminal_session_id, :is_remote::boolean, "
           ":event_data, :created_at, :created_by, :updated_at, "
           ":updated_by) "
           "RETURNING id";
}

QString SessionEventRepository::buildUpdateQuery()
{
    return "UPDATE session_events SET "
           "session_id = :session_id, "
           "event_type = :event_type, "
           "event_time = :event_time, "
           "user_id = :user_id, "
           "previous_user_id = :previous_user_id, "
           "machine_id = :machine_id, "
           "terminal_session_id = :terminal_session_id, "
           "is_remote = :is_remote::boolean, "
           "event_data = :event_data, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE event_id = :event_id";
}

QString SessionEventRepository::buildGetByIdQuery()
{
    return "SELECT * FROM session_events WHERE event_id = :event_id";
}

QString SessionEventRepository::buildGetAllQuery()
{
    return "SELECT * FROM session_events ORDER BY event_time DESC";
}

QString SessionEventRepository::buildRemoveQuery()
{
    return "DELETE FROM session_events WHERE event_id = :event_id";
}

QMap<QString, QVariant> SessionEventRepository::prepareParamsForSave(SessionEventModel* event)
{
    QMap<QString, QVariant> params;
    params["session_id"] = event->sessionId().toString(QUuid::WithoutBraces);
    params["event_type"] = eventTypeToString(event->eventType());
    params["event_time"] = event->eventTime().toUTC();
    params["user_id"] = event->userId().isNull() ? QVariant(QVariant::Invalid) : event->userId().toString(QUuid::WithoutBraces);
    params["previous_user_id"] = event->previousUserId().isNull() ? QVariant(QVariant::Invalid) : event->previousUserId().toString(QUuid::WithoutBraces);
    params["machine_id"] = event->machineId().toString(QUuid::WithoutBraces);
    params["terminal_session_id"] = event->terminalSessionId();
    params["is_remote"] = event->isRemote() ? "true" : "false";
    params["event_data"] = QString::fromUtf8(QJsonDocument(event->eventData()).toJson());
    params["created_at"] = event->createdAt().toUTC();
    params["created_by"] = event->createdBy().isNull() ? QVariant(QVariant::Invalid) : event->createdBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = event->updatedAt().toUTC();
    params["updated_by"] = event->updatedBy().isNull() ? QVariant(QVariant::Invalid) : event->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

QMap<QString, QVariant> SessionEventRepository::prepareParamsForUpdate(SessionEventModel* event)
{
    QMap<QString, QVariant> params = prepareParamsForSave(event);
    params["event_id"] = event->id().toString(QUuid::WithoutBraces);
    return params;
}

SessionEventModel* SessionEventRepository::createModelFromQuery(const QSqlQuery &query)
{
    return ModelFactory::createSessionEventFromQuery(query);
}

QList<QSharedPointer<SessionEventModel>> SessionEventRepository::getBySessionId(const QUuid &sessionId, int limit, int offset)
{
    LOG_DEBUG(QString("Getting session events by session ID: %1 (limit: %2, offset: %3)")
              .arg(sessionId.toString())
              .arg(limit)
              .arg(offset));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<SessionEventModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString queryStr = "SELECT * FROM session_events WHERE session_id = :session_id ORDER BY event_time DESC";

    // Add limit and offset if provided
    if (limit > 0) {
        queryStr += QString(" LIMIT %1").arg(limit);

        if (offset > 0) {
            queryStr += QString(" OFFSET %1").arg(offset);
        }
    }

    QVector<SessionEventModel*> events = m_dbService->executeSelectQuery(
        queryStr,
        params,
        [this](const QSqlQuery& query) -> SessionEventModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<SessionEventModel>> result;
    for (auto event : events) {
        result.append(QSharedPointer<SessionEventModel>(event));
    }

    LOG_INFO(QString("Retrieved %1 session events for session %2")
             .arg(result.size())
             .arg(sessionId.toString()));
    return result;
}

QList<QSharedPointer<SessionEventModel>> SessionEventRepository::getByEventType(
    const QUuid &sessionId,
    EventTypes::SessionEventType eventType,
    int limit,
    int offset)
{
    LOG_DEBUG(QString("Getting session events by event type for session ID: %1 (type: %2, limit: %3, offset: %4)")
              .arg(sessionId.toString())
              .arg(eventTypeToString(eventType))
              .arg(limit)
              .arg(offset));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<SessionEventModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);
    params["event_type"] = eventTypeToString(eventType);

    QString queryStr = "SELECT * FROM session_events WHERE session_id = :session_id AND event_type = :event_type ORDER BY event_time DESC";

    // Add limit and offset if provided
    if (limit > 0) {
        queryStr += QString(" LIMIT %1").arg(limit);

        if (offset > 0) {
            queryStr += QString(" OFFSET %1").arg(offset);
        }
    }

    QVector<SessionEventModel*> events = m_dbService->executeSelectQuery(
        queryStr,
        params,
        [this](const QSqlQuery& query) -> SessionEventModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<SessionEventModel>> result;
    for (auto event : events) {
        result.append(QSharedPointer<SessionEventModel>(event));
    }

    LOG_INFO(QString("Retrieved %1 session events of type %2 for session %3")
             .arg(result.size())
             .arg(eventTypeToString(eventType))
             .arg(sessionId.toString()));
    return result;
}

QList<QSharedPointer<SessionEventModel>> SessionEventRepository::getByTimeRange(
    const QUuid &sessionId,
    const QDateTime &startTime,
    const QDateTime &endTime,
    int limit,
    int offset)
{
    LOG_DEBUG(QString("Getting session events by time range for session ID: %1 (limit: %2, offset: %3)")
              .arg(sessionId.toString())
              .arg(limit)
              .arg(offset));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<SessionEventModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);
    params["start_time"] = startTime.toUTC();
    params["end_time"] = endTime.toUTC();

    QString queryStr = "SELECT * FROM session_events WHERE session_id = :session_id AND event_time >= :start_time AND event_time <= :end_time ORDER BY event_time DESC";

    // Add limit and offset if provided
    if (limit > 0) {
        queryStr += QString(" LIMIT %1").arg(limit);

        if (offset > 0) {
            queryStr += QString(" OFFSET %1").arg(offset);
        }
    }

    QVector<SessionEventModel*> events = m_dbService->executeSelectQuery(
        queryStr,
        params,
        [this](const QSqlQuery& query) -> SessionEventModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<SessionEventModel>> result;
    for (auto event : events) {
        result.append(QSharedPointer<SessionEventModel>(event));
    }

    LOG_INFO(QString("Retrieved %1 session events in time range for session %2")
             .arg(result.size())
             .arg(sessionId.toString()));
    return result;
}

QList<QSharedPointer<SessionEventModel>> SessionEventRepository::getByUserId(const QUuid &userId, int limit, int offset)
{
    LOG_DEBUG(QString("Getting session events by user ID: %1 (limit: %2, offset: %3)")
              .arg(userId.toString())
              .arg(limit)
              .arg(offset));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<SessionEventModel>>();
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);

    QString queryStr = "SELECT * FROM session_events WHERE user_id = :user_id ORDER BY event_time DESC";

    // Add limit and offset if provided
    if (limit > 0) {
        queryStr += QString(" LIMIT %1").arg(limit);

        if (offset > 0) {
            queryStr += QString(" OFFSET %1").arg(offset);
        }
    }

    QVector<SessionEventModel*> events = m_dbService->executeSelectQuery(
        queryStr,
        params,
        [this](const QSqlQuery& query) -> SessionEventModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<SessionEventModel>> result;
    for (auto event : events) {
        result.append(QSharedPointer<SessionEventModel>(event));
    }

    LOG_INFO(QString("Retrieved %1 session events for user %2")
             .arg(result.size())
             .arg(userId.toString()));
    return result;
}

QList<QSharedPointer<SessionEventModel>> SessionEventRepository::getByMachineId(const QString &machineId, int limit, int offset)
{
    LOG_DEBUG(QString("Getting session events by machine ID: %1 (limit: %2, offset: %3)")
              .arg(machineId)
              .arg(limit)
              .arg(offset));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<SessionEventModel>>();
    }

    QMap<QString, QVariant> params;
    params["machine_id"] = machineId;

    QString queryStr = "SELECT * FROM session_events WHERE machine_id = :machine_id ORDER BY event_time DESC";

    // Add limit and offset if provided
    if (limit > 0) {
        queryStr += QString(" LIMIT %1").arg(limit);

        if (offset > 0) {
            queryStr += QString(" OFFSET %1").arg(offset);
        }
    }

    QVector<SessionEventModel*> events = m_dbService->executeSelectQuery(
        queryStr,
        params,
        [this](const QSqlQuery& query) -> SessionEventModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<SessionEventModel>> result;
    for (auto event : events) {
        result.append(QSharedPointer<SessionEventModel>(event));
    }

    LOG_INFO(QString("Retrieved %1 session events for machine %2")
             .arg(result.size())
             .arg(machineId));
    return result;
}

QJsonObject SessionEventRepository::getSessionEventSummary(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting session event summary for session ID: %1").arg(sessionId.toString()));

    QJsonObject summary;

    if (!ensureInitialized()) {
        return summary;
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    // Get event counts by type
    QString countQuery =
        "SELECT event_type, COUNT(*) as count "
        "FROM session_events "
        "WHERE session_id = :session_id "
        "GROUP BY event_type";

    QJsonObject eventCounts;
    int totalEvents = 0;

    auto countResults = m_dbService->executeSelectQuery(
        countQuery,
        params,
        [&eventCounts, &totalEvents](const QSqlQuery& query) -> SessionEventModel* {
            if (query.isValid()) {
                QString eventType = query.value("event_type").toString();
                int count = query.value("count").toInt();
                eventCounts[eventType] = count;
                totalEvents += count;
            }
            return nullptr; // We're not creating models here, just collecting counts
        }
    );

    summary["total_events"] = totalEvents;
    summary["event_counts"] = eventCounts;

    // Get first and last event times
    QString timeQuery =
        "SELECT MIN(event_time) as first_event, MAX(event_time) as last_event "
        "FROM session_events "
        "WHERE session_id = :session_id";

    m_dbService->executeSingleSelectQuery(
        timeQuery,
        params,
        [&summary](const QSqlQuery& query) -> SessionEventModel* {
            if (query.isValid()) {
                QDateTime firstEvent = query.value("first_event").toDateTime();
                QDateTime lastEvent = query.value("last_event").toDateTime();

                if (firstEvent.isValid() && lastEvent.isValid()) {
                    summary["first_event"] = firstEvent.toUTC().toString();
                    summary["last_event"] = lastEvent.toUTC().toString();
                    summary["duration_seconds"] = firstEvent.secsTo(lastEvent);
                }
            }
            return nullptr; // Not creating a model
        }
    );

    LOG_INFO(QString("Retrieved session event summary for session %1 (total events: %2)")
             .arg(sessionId.toString())
             .arg(totalEvents));
    return summary;
}

QString SessionEventRepository::eventTypeToString(EventTypes::SessionEventType eventType)
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
            LOG_WARNING(QString("Unknown event type ID: %1, mapping to 'unknown'").arg(static_cast<int>(eventType)));
            return "unknown";
    }
}

EventTypes::SessionEventType SessionEventRepository::stringToEventType(const QString &eventTypeStr)
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
        LOG_WARNING(QString("Unknown event type string: '%1', defaulting to Login").arg(eventTypeStr));
        return EventTypes::SessionEventType::Login;
    }
}

// Find the save method which should look something like this
bool SessionEventRepository::save(SessionEventModel* event)
{
    if (!ensureInitialized()) {
        LOG_ERROR("Cannot save SessionEvent: repository not initialized");
        return false;
    }

    // Validate event before saving
    QStringList validationErrors;
    if (!validateModel(event, validationErrors)) {
        LOG_ERROR(QString("Cannot save SessionEvent: validation failed - %1")
                 .arg(validationErrors.join(", ")));
        return false;
    }

    // Prepare query parameters
    QMap<QString, QVariant> params = prepareParamsForSave(event);

    // Build query
    QString query = buildSaveQuery();

    // Enhanced debugging - log entire query with parameters
    logQueryWithValues(query, params);

    // Execute query
    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("SessionEvent saved successfully: %1").arg(event->id().toString()));
    } else {
        // Enhanced error logging
        LOG_ERROR(QString("Failed to save SessionEvent: %1").arg(event->id().toString()));
        LOG_ERROR(QString("Database error: %1").arg(m_dbService->lastError()));

        // Log specific parameter information that might be causing issues
        LOG_ERROR("Checking critical parameters:");
        LOG_ERROR(QString("- session_id: %1").arg(params["session_id"].toString()));
        LOG_ERROR(QString("- event_type: %1").arg(params["event_type"].toString()));
        LOG_ERROR(QString("- event_time: %1").arg(params["event_time"].toString()));

        // Check DB connection
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            LOG_ERROR("Database connection is closed!");
        } else {
            LOG_ERROR(QString("Database connection is open. Name: %1, Driver: %2")
                     .arg(db.connectionName(), db.driverName()));

            // Try to check if the session_events table exists
            QStringList tables = db.tables();
            LOG_ERROR(QString("Found tables: %1").arg(tables.join(", ")));
            LOG_ERROR(QString("session_events table exists: %1").arg(tables.contains("session_events") ? "yes" : "NO"));
        }
    }

    return success;
}

