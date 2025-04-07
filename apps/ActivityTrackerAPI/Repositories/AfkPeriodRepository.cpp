#include "AfkPeriodRepository.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "logger/logger.h"
#include "Core/ModelFactory.h"

AfkPeriodRepository::AfkPeriodRepository(QObject *parent)
    : BaseRepository<AfkPeriodModel>(parent)
{
    LOG_DEBUG("AfkPeriodRepository created");
}

QString AfkPeriodRepository::getEntityName() const
{
    return "AfkPeriod";
}

QString AfkPeriodRepository::getModelId(AfkPeriodModel* model) const
{
    return model->id().toString();
}

QString AfkPeriodRepository::buildSaveQuery()
{
    return "INSERT INTO afk_periods "
           "(session_id, start_time, end_time, created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:session_id, :start_time, :end_time, :created_at, :created_by, :updated_at, :updated_by) "
           "RETURNING id";
}

QString AfkPeriodRepository::buildUpdateQuery()
{
    return "UPDATE afk_periods SET "
           "session_id = :session_id, "
           "start_time = :start_time, "
           "end_time = :end_time, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE afk_id = :afk_id";
}

QString AfkPeriodRepository::buildGetByIdQuery()
{
    return "SELECT * FROM afk_periods WHERE afk_id = :afk_id";
}

QString AfkPeriodRepository::buildGetAllQuery()
{
    return "SELECT * FROM afk_periods ORDER BY start_time DESC";
}

QString AfkPeriodRepository::buildRemoveQuery()
{
    return "DELETE FROM afk_periods WHERE afk_id = :afk_id";
}

QMap<QString, QVariant> AfkPeriodRepository::prepareParamsForSave(AfkPeriodModel* afkPeriod)
{
    QMap<QString, QVariant> params;
    params["session_id"] = afkPeriod->sessionId().toString(QUuid::WithoutBraces);
    params["start_time"] = afkPeriod->startTime().toUTC();
    params["end_time"] = afkPeriod->endTime().isValid() ? afkPeriod->endTime().toUTC() : QVariant(QVariant::Invalid);
    params["created_at"] = afkPeriod->createdAt().toUTC();
    params["created_by"] = afkPeriod->createdBy().isNull() ? QVariant(QVariant::Invalid) : afkPeriod->createdBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = afkPeriod->updatedAt().toUTC();
    params["updated_by"] = afkPeriod->updatedBy().isNull() ? QVariant(QVariant::Invalid) : afkPeriod->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

QMap<QString, QVariant> AfkPeriodRepository::prepareParamsForUpdate(AfkPeriodModel* afkPeriod)
{
    QMap<QString, QVariant> params = prepareParamsForSave(afkPeriod);
    params["afk_id"] = afkPeriod->id().toString(QUuid::WithoutBraces);
    return params;
}

AfkPeriodModel* AfkPeriodRepository::createModelFromQuery(const QSqlQuery &query)
{
    // Use ModelFactory to create the model
    return ModelFactory::createAfkPeriodFromQuery(query);
}

QList<QSharedPointer<AfkPeriodModel>> AfkPeriodRepository::getBySessionId(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting AFK periods by session ID: %1").arg(sessionId.toString()));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<AfkPeriodModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM afk_periods WHERE session_id = :session_id ORDER BY start_time DESC";

    auto afkPeriods = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> AfkPeriodModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<AfkPeriodModel>> result;
    for (auto afkPeriod : afkPeriods) {
        result.append(QSharedPointer<AfkPeriodModel>(afkPeriod));
    }

    LOG_INFO(QString("Retrieved %1 AFK periods for session %2")
            .arg(result.size()).arg(sessionId.toString()));
    return result;
}

QList<QSharedPointer<AfkPeriodModel>> AfkPeriodRepository::getActiveAfkPeriods(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting active AFK periods for session: %1").arg(sessionId.toString()));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<AfkPeriodModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM afk_periods WHERE session_id = :session_id AND end_time IS NULL ORDER BY start_time DESC";

    auto afkPeriods = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> AfkPeriodModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<AfkPeriodModel>> result;
    for (auto afkPeriod : afkPeriods) {
        result.append(QSharedPointer<AfkPeriodModel>(afkPeriod));
    }

    LOG_INFO(QString("Retrieved %1 active AFK periods for session %2")
            .arg(result.size()).arg(sessionId.toString()));
    return result;
}

QSharedPointer<AfkPeriodModel> AfkPeriodRepository::getLastAfkPeriod(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting last AFK period for session: %1").arg(sessionId.toString()));

    if (!ensureInitialized()) {
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM afk_periods WHERE session_id = :session_id ORDER BY start_time DESC LIMIT 1";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> AfkPeriodModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("Last AFK period found for session: %1").arg(sessionId.toString()));
        return QSharedPointer<AfkPeriodModel>(*result);
    } else {
        LOG_WARNING(QString("No AFK periods found for session: %1").arg(sessionId.toString()));
        return nullptr;
    }
}

bool AfkPeriodRepository::endAfkPeriod(const QUuid &afkId, const QDateTime &endTime)
{
    LOG_DEBUG(QString("Ending AFK period: %1 at %2")
             .arg(afkId.toString(), endTime.toUTC().toString()));

    if (!ensureInitialized()) {
        return false;
    }

    QMap<QString, QVariant> params;
    params["afk_id"] = afkId.toString(QUuid::WithoutBraces);
    params["end_time"] = endTime.toUTC();
    params["updated_at"] = QDateTime::currentDateTimeUtc().toUTC();

    QString query =
        "UPDATE afk_periods SET "
        "end_time = :end_time, "
        "updated_at = :updated_at "
        "WHERE afk_id = :afk_id";

    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("AFK period ended successfully: %1").arg(afkId.toString()));
    } else {
        LOG_ERROR(QString("Failed to end AFK period: %1 - %2")
                .arg(afkId.toString(), m_dbService->lastError()));
    }

    return success;
}

QJsonObject AfkPeriodRepository::getAfkSummary(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting AFK summary for session: %1").arg(sessionId.toString()));

    QJsonObject summary;

    if (!ensureInitialized()) {
        return summary;
    }

    // Define a helper class just for this specific query
    class StatsHelper : public AfkPeriodModel {
    public:
        StatsHelper() : AfkPeriodModel(), totalAfk(0), totalAfkSeconds(0) {}
        int totalAfk;
        double totalAfkSeconds;
    };

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT "
        "COUNT(*) as total_afk, "
        "COALESCE(SUM(EXTRACT(EPOCH FROM (COALESCE(end_time, CURRENT_TIMESTAMP) - start_time))), 0) as total_afk_seconds "
        "FROM afk_periods "
        "WHERE session_id = :session_id";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> AfkPeriodModel* {
            // Create our local helper model
            StatsHelper* helper = new StatsHelper();

            // Store the stats in our helper
            helper->totalAfk = query.value("total_afk").toInt();
            helper->totalAfkSeconds = query.value("total_afk_seconds").toDouble();

            return helper;
        }
    );

    if (result) {
        // Cast to our helper type
        StatsHelper* helper = static_cast<StatsHelper*>(*result);

        // Build the JSON response
        summary["total_afk"] = helper->totalAfk;
        summary["total_afk_seconds"] = helper->totalAfkSeconds;

        LOG_INFO(QString("Retrieved AFK summary for session %1: %2 periods, %3 seconds")
                .arg(sessionId.toString())
                .arg(helper->totalAfk)
                .arg(helper->totalAfkSeconds));

        delete *result; // Clean up
    } else {
        // Set default values if no data found
        summary["total_afk"] = 0;
        summary["total_afk_seconds"] = 0.0;

        LOG_INFO(QString("No AFK data found for session %1").arg(sessionId.toString()));
    }

    return summary;
}

