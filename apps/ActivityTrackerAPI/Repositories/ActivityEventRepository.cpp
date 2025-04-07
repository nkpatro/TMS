#include "ActivityEventRepository.h"
#include <QJsonDocument>
#include "logger/logger.h"
#include "Core/ModelFactory.h"

ActivityEventRepository::ActivityEventRepository(QObject *parent)
    : BaseRepository<ActivityEventModel>(parent)
{
    LOG_DEBUG("ActivityEventRepository created");
}

QString ActivityEventRepository::getEntityName() const
{
    return "ActivityEvent";
}

QString ActivityEventRepository::getModelId(ActivityEventModel* model) const
{
    return model->id().toString();
}

QString ActivityEventRepository::buildSaveQuery()
{
    return "INSERT INTO activity_events "
           "(session_id, app_id, event_type, event_time, event_data, created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:session_id, :app_id, :event_type, :event_time, :event_data::jsonb, "
           ":created_at, :created_by, :updated_at, :updated_by) "
           "RETURNING id";
}

QString ActivityEventRepository::buildUpdateQuery()
{
    return "UPDATE activity_events SET "
           "session_id = :session_id, "
           "app_id = :app_id, "
           "event_type = :event_type, "
           "event_time = :event_time, "
           "event_data = :event_data::jsonb, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE event_id = :event_id";
}

QString ActivityEventRepository::buildGetByIdQuery()
{
    return "SELECT * FROM activity_events WHERE event_id = :event_id";
}

QString ActivityEventRepository::buildGetAllQuery()
{
    return "SELECT * FROM activity_events ORDER BY event_time DESC";
}

QString ActivityEventRepository::buildRemoveQuery()
{
    return "DELETE FROM activity_events WHERE event_id = :event_id";
}

QMap<QString, QVariant> ActivityEventRepository::prepareParamsForSave(ActivityEventModel* event)
{
    QMap<QString, QVariant> params;
    params["session_id"] = event->sessionId().toString(QUuid::WithoutBraces);
    params["app_id"] = event->appId().isNull() ? QVariant(QVariant::Invalid) : event->appId().toString(QUuid::WithoutBraces);
    params["event_type"] = eventTypeToString(event->eventType());
    params["event_time"] = event->eventTime().toUTC();
    params["event_data"] = QString(QJsonDocument(event->eventData()).toJson());
    params["created_at"] = event->createdAt().toUTC();
    params["created_by"] = event->createdBy().isNull() ? QVariant(QVariant::Invalid) : event->createdBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = event->updatedAt().toUTC();
    params["updated_by"] = event->updatedBy().isNull() ? QVariant(QVariant::Invalid) : event->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

QMap<QString, QVariant> ActivityEventRepository::prepareParamsForUpdate(ActivityEventModel* event)
{
    QMap<QString, QVariant> params = prepareParamsForSave(event);
    params["event_id"] = event->id().toString(QUuid::WithoutBraces);
    return params;
}

ActivityEventModel* ActivityEventRepository::createModelFromQuery(const QSqlQuery &query)
{
    // Use ModelFactory to create the model
    return ModelFactory::createActivityEventFromQuery(query);
}

QList<QSharedPointer<ActivityEventModel>> ActivityEventRepository::getBySessionId(const QUuid &sessionId, int limit, int offset)
{
    LOG_DEBUG(QString("Getting activity events by session ID: %1 (limit: %2, offset: %3)")
             .arg(sessionId.toString()).arg(limit).arg(offset));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<ActivityEventModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM activity_events WHERE session_id = :session_id ORDER BY event_time DESC";

    if (limit > 0) {
        query += " LIMIT :limit";
        params["limit"] = QString::number(limit);
    }

    if (offset > 0) {
        query += " OFFSET :offset";
        params["offset"] = QString::number(offset);
    }

    QVector<ActivityEventModel*> events = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> ActivityEventModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<ActivityEventModel>> result;
    for (auto event : events) {
        result.append(QSharedPointer<ActivityEventModel>(event));
    }

    LOG_INFO(QString("Retrieved %1 activity events for session %2")
            .arg(result.size()).arg(sessionId.toString()));
    return result;
}

QList<QSharedPointer<ActivityEventModel>> ActivityEventRepository::getByApplicationId(const QUuid &appId, int limit, int offset)
{
    LOG_DEBUG(QString("Getting activity events by application ID: %1 (limit: %2, offset: %3)")
             .arg(appId.toString()).arg(limit).arg(offset));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<ActivityEventModel>>();
    }

    QMap<QString, QVariant> params;
    params["app_id"] = appId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM activity_events WHERE app_id = :app_id ORDER BY event_time DESC";

    if (limit > 0) {
        query += " LIMIT :limit";
        params["limit"] = QString::number(limit);
    }

    if (offset > 0) {
        query += " OFFSET :offset";
        params["offset"] = QString::number(offset);
    }

    QVector<ActivityEventModel*> events = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> ActivityEventModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<ActivityEventModel>> result;
    for (auto event : events) {
        result.append(QSharedPointer<ActivityEventModel>(event));
    }

    LOG_INFO(QString("Retrieved %1 activity events for application %2")
            .arg(result.size()).arg(appId.toString()));
    return result;
}

QList<QSharedPointer<ActivityEventModel>> ActivityEventRepository::getByEventType(
    const QUuid &sessionId,
    EventTypes::ActivityEventType eventType,
    int limit,
    int offset)
{
    LOG_DEBUG(QString("Getting activity events by type: %1 for session: %2")
             .arg(eventTypeToString(eventType), sessionId.toString()));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<ActivityEventModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);
    params["event_type"] = eventTypeToString(eventType);

    QString query = "SELECT * FROM activity_events WHERE session_id = :session_id AND event_type = :event_type ORDER BY event_time DESC";

    if (limit > 0) {
        query += " LIMIT :limit";
        params["limit"] = QString::number(limit);
    }

    if (offset > 0) {
        query += " OFFSET :offset";
        params["offset"] = QString::number(offset);
    }

    QVector<ActivityEventModel*> events = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> ActivityEventModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<ActivityEventModel>> result;
    for (auto event : events) {
        result.append(QSharedPointer<ActivityEventModel>(event));
    }

    LOG_INFO(QString("Retrieved %1 activity events of type %2 for session %3")
            .arg(result.size()).arg(eventTypeToString(eventType)).arg(sessionId.toString()));
    return result;
}

QList<QSharedPointer<ActivityEventModel>> ActivityEventRepository::getByTimeRange(
    const QUuid &sessionId,
    const QDateTime &startTime,
    const QDateTime &endTime,
    int limit,
    int offset)
{
    LOG_DEBUG(QString("Getting activity events by time range for session: %1")
             .arg(sessionId.toString()));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<ActivityEventModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);
    params["start_time"] = startTime.toUTC();
    params["end_time"] = endTime.toUTC();

    QString query = "SELECT * FROM activity_events WHERE session_id = :session_id "
                  "AND event_time >= :start_time AND event_time <= :end_time ORDER BY event_time DESC";

    if (limit > 0) {
        query += " LIMIT :limit";
        params["limit"] = QString::number(limit);
    }

    if (offset > 0) {
        query += " OFFSET :offset";
        params["offset"] = QString::number(offset);
    }

    QVector<ActivityEventModel*> events = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> ActivityEventModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<ActivityEventModel>> result;
    for (auto event : events) {
        result.append(QSharedPointer<ActivityEventModel>(event));
    }

    LOG_INFO(QString("Retrieved %1 activity events in time range for session %2")
            .arg(result.size()).arg(sessionId.toString()));
    return result;
}

int ActivityEventRepository::getEventCountByType(const QUuid &sessionId, EventTypes::ActivityEventType eventType)
{
    LOG_DEBUG(QString("Getting count of activity events by type: %1 for session: %2")
             .arg(eventTypeToString(eventType)).arg(sessionId.toString()));

    if (!ensureInitialized()) {
        return 0;
    }

    // Create a class or struct to hold the count value
    class CountResult : public ActivityEventModel {
    public:
        int count;
        CountResult(int val) : count(val) {}
    };

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);
    params["event_type"] = eventTypeToString(eventType);

    QString query = "SELECT COUNT(*) as count FROM activity_events "
                  "WHERE session_id = :session_id AND event_type = :event_type";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> ActivityEventModel* {
            // Create a CountResult that derives from ActivityEventModel
            return new CountResult(query.value("count").toInt());
        }
    );

    if (result) {
        // Cast to CountResult* and extract the count
        CountResult* countResult = static_cast<CountResult*>(*result);
        int count = countResult->count;
        delete countResult; // Clean up

        LOG_INFO(QString("Event count for type %1 in session %2: %3")
                .arg(eventTypeToString(eventType)).arg(sessionId.toString()).arg(count));

        return count;
    } else {
        LOG_WARNING(QString("Failed to get event count for type %1 in session %2")
                   .arg(eventTypeToString(eventType)).arg(sessionId.toString()));
        return 0;
    }
}

QJsonObject ActivityEventRepository::getActivitySummary(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting activity summary for session: %1").arg(sessionId.toString()));

    QJsonObject summary;

    if (!ensureInitialized()) {
        return summary;
    }

    // Get total events
    QMap<QString, QVariant> totalParams;
    totalParams["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString totalQuery = "SELECT COUNT(*) as total FROM activity_events WHERE session_id = :session_id";

    auto totalResult = m_dbService->executeSingleSelectQuery(
        totalQuery,
        totalParams,
        [](const QSqlQuery& query) -> ActivityEventModel* {
            QJsonObject data;
            data["total"] = query.value("total").toInt();

            // Create a base ActivityEventModel and store the data in its eventData
            ActivityEventModel* model = ModelFactory::createDefaultActivityEvent();
            model->setEventData(data);
            return model;
        }
    );

    if (totalResult) {
        // Extract the total from the event data
        summary["total_events"] = (*totalResult)->eventData()["total"].toInt();
        delete *totalResult;
    } else {
        summary["total_events"] = 0;
    }

    // Get event counts by type
    QMap<QString, QVariant> typeParams;
    typeParams["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString typeQuery = "SELECT event_type, COUNT(*) as count FROM activity_events "
                       "WHERE session_id = :session_id GROUP BY event_type";

    QJsonObject eventCounts;
    QVector<ActivityEventModel*> typeCounts = m_dbService->executeSelectQuery(
        typeQuery,
        typeParams,
        [](const QSqlQuery& query) -> ActivityEventModel* {
            QJsonObject data;
            data["event_type"] = query.value("event_type").toString();
            data["count"] = query.value("count").toInt();

            ActivityEventModel* model = ModelFactory::createDefaultActivityEvent();
            model->setEventData(data);
            return model;
        }
    );

    for (auto model : typeCounts) {
        QJsonObject data = model->eventData();
        eventCounts[data["event_type"].toString()] = data["count"].toInt();
        delete model;
    }

    summary["event_counts"] = eventCounts;

    // Get first and last event times
    QMap<QString, QVariant> timeParams;
    timeParams["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString timeQuery = "SELECT MIN(event_time) as first_event, MAX(event_time) as last_event "
                       "FROM activity_events WHERE session_id = :session_id";

    auto timeResult = m_dbService->executeSingleSelectQuery(
        timeQuery,
        timeParams,
        [](const QSqlQuery& query) -> ActivityEventModel* {
            QJsonObject data;
            data["first_event"] = query.value("first_event").toDateTime().toUTC().toString();
            data["last_event"] = query.value("last_event").toDateTime().toUTC().toString();

            // Store the original QDateTime values for calculation
            if (!query.value("first_event").isNull() && !query.value("last_event").isNull()) {
                QDateTime firstEvent = query.value("first_event").toDateTime();
                QDateTime lastEvent = query.value("last_event").toDateTime();
                data["duration_seconds"] = firstEvent.secsTo(lastEvent);
            }

            ActivityEventModel* model = ModelFactory::createDefaultActivityEvent();
            model->setEventData(data);
            return model;
        }
    );

    if (timeResult) {
        QJsonObject timeData = (*timeResult)->eventData();

        if (timeData.contains("first_event") && timeData.contains("last_event")) {
            summary["first_event"] = timeData["first_event"].toString();
            summary["last_event"] = timeData["last_event"].toString();

            if (timeData.contains("duration_seconds")) {
                summary["duration_seconds"] = timeData["duration_seconds"].toInt();
            }
        }

        delete *timeResult;
    }

    LOG_INFO(QString("Generated activity summary for session %1").arg(sessionId.toString()));
    return summary;
}

QString ActivityEventRepository::eventTypeToString(EventTypes::ActivityEventType eventType)
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
            return "unknown";
    }
}

EventTypes::ActivityEventType ActivityEventRepository::stringToEventType(const QString &eventTypeStr)
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
        // Default to MouseClick if unknown
        LOG_WARNING(QString("Unknown activity event type: %1, defaulting to MouseClick").arg(eventTypeStr));
        return EventTypes::ActivityEventType::MouseClick;
    }
}

