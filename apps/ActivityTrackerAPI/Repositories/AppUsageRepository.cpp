#include "AppUsageRepository.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include "Core/ModelFactory.h"

AppUsageRepository::AppUsageRepository(QObject *parent)
    : BaseRepository<AppUsageModel>(parent)
{
    LOG_DEBUG("AppUsageRepository created");
}

QString AppUsageRepository::getEntityName() const
{
    return "AppUsage";
}

QString AppUsageRepository::getModelId(AppUsageModel* model) const
{
    return model->id().toString();
}

QString AppUsageRepository::buildSaveQuery()
{
    return "INSERT INTO app_usage "
           "(session_id, app_id, start_time, end_time, is_active, window_title, "
           "created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:session_id, :app_id, :start_time, :end_time, :is_active, :window_title, "
           ":created_at, :created_by, :updated_at, :updated_by) "
           "RETURNING id";
}

QString AppUsageRepository::buildUpdateQuery()
{
    return "UPDATE app_usage SET "
           "session_id = :session_id, "
           "app_id = :app_id, "
           "start_time = :start_time, "
           "end_time = :end_time, "
           "is_active = :is_active, "
           "window_title = :window_title, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE id = :id";
}

QString AppUsageRepository::buildGetByIdQuery()
{
    return "SELECT * FROM app_usage WHERE id = :id";
}

QString AppUsageRepository::buildGetAllQuery()
{
    return "SELECT * FROM app_usage ORDER BY start_time DESC";
}

QString AppUsageRepository::buildRemoveQuery()
{
    return "DELETE FROM app_usage WHERE id = :id";
}

QString AppUsageRepository::getIdParamName() const
{
    return "id";
}

QMap<QString, QVariant> AppUsageRepository::prepareParamsForSave(AppUsageModel* appUsage)
{
    QMap<QString, QVariant> params;
    params["session_id"] = appUsage->sessionId().toString(QUuid::WithoutBraces);
    params["app_id"] = appUsage->appId().toString(QUuid::WithoutBraces);
    params["start_time"] = appUsage->startTime().toString(Qt::ISODate);
    params["end_time"] = appUsage->endTime().isValid() ? appUsage->endTime().toString(Qt::ISODate) : QVariant(QVariant::Invalid);
    params["is_active"] = appUsage->isActive() ? "true" : "false";
    params["window_title"] = appUsage->windowTitle();
    params["created_at"] = appUsage->createdAt().toString(Qt::ISODate);
    params["created_by"] = appUsage->createdBy().isNull() ? QVariant(QVariant::Invalid) : appUsage->createdBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = appUsage->updatedAt().toString(Qt::ISODate);
    params["updated_by"] = appUsage->updatedBy().isNull() ? QVariant(QVariant::Invalid) : appUsage->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

QMap<QString, QVariant> AppUsageRepository::prepareParamsForUpdate(AppUsageModel* appUsage)
{
    QMap<QString, QVariant> params = prepareParamsForSave(appUsage);
    params["id"] = appUsage->id().toString(QUuid::WithoutBraces);

    return params;
}

AppUsageModel* AppUsageRepository::createModelFromQuery(const QSqlQuery &query)
{
    // Use ModelFactory to create the model
    return ModelFactory::createAppUsageFromQuery(query);
}

bool AppUsageRepository::validateModel(AppUsageModel* model, QStringList& errors)
{
    return ModelFactory::validateAppUsageModel(model, errors);
}

bool AppUsageRepository::endAppUsage(const QUuid &usageId, const QDateTime &endTime)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot end app usage: Repository not initialized");
        return false;
    }

    QMap<QString, QVariant> params;
    params["id"] = usageId.toString(QUuid::WithoutBraces);
    params["end_time"] = endTime.toString(Qt::ISODate);
    params["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QString query =
        "UPDATE app_usage SET "
        "end_time = :end_time, "
        "is_active = false, "
        "updated_at = :updated_at "
        "WHERE id = :id";

    logQueryWithValues(query, params);
    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("App usage ended successfully: %1").arg(usageId.toString()));
    } else {
        LOG_ERROR(QString("Failed to end app usage: %1").arg(usageId.toString()));
    }

    return success;
}

QList<QSharedPointer<AppUsageModel>> AppUsageRepository::getBySessionId(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting app usages for session: %1").arg(sessionId.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get app usages: Repository not initialized");
        return QList<QSharedPointer<AppUsageModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM app_usage WHERE session_id = :session_id ORDER BY start_time DESC";

    auto appUsages = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> AppUsageModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<AppUsageModel>> result;
    for (auto usage : appUsages) {
        result.append(QSharedPointer<AppUsageModel>(usage));
    }

    LOG_INFO(QString("Retrieved %1 app usage records for session %2").arg(appUsages.size()).arg(sessionId.toString()));
    return result;
}

QList<QSharedPointer<AppUsageModel>> AppUsageRepository::getByAppId(const QUuid &appId)
{
    LOG_DEBUG(QString("Getting app usages for app: %1").arg(appId.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get app usages: Repository not initialized");
        return QList<QSharedPointer<AppUsageModel>>();
    }

    QMap<QString, QVariant> params;
    params["app_id"] = appId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM app_usage WHERE app_id = :app_id ORDER BY start_time DESC";

    auto appUsages = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> AppUsageModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<AppUsageModel>> result;
    for (auto usage : appUsages) {
        result.append(QSharedPointer<AppUsageModel>(usage));
    }

    LOG_INFO(QString("Retrieved %1 app usage records for app %2").arg(appUsages.size()).arg(appId.toString()));
    return result;
}

QList<QSharedPointer<AppUsageModel>> AppUsageRepository::getActiveAppUsages(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting active app usages for session: %1").arg(sessionId.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get active app usages: Repository not initialized");
        return QList<QSharedPointer<AppUsageModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM app_usage WHERE session_id = :session_id AND is_active = true ORDER BY start_time DESC";

    auto appUsages = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> AppUsageModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<AppUsageModel>> result;
    for (auto usage : appUsages) {
        result.append(QSharedPointer<AppUsageModel>(usage));
    }

    LOG_INFO(QString("Retrieved %1 active app usage records for session %2").arg(result.size()).arg(sessionId.toString()));
    return result;
}

QSharedPointer<AppUsageModel> AppUsageRepository::getCurrentActiveApp(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting current active app for session: %1").arg(sessionId.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get current active app: Repository not initialized");
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM app_usage WHERE session_id = :session_id AND is_active = true "
                   "ORDER BY start_time DESC LIMIT 1";

    logQueryWithValues(query, params);
    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> AppUsageModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO("Current active app found");
        return QSharedPointer<AppUsageModel>(*result);
    }

    LOG_WARNING("No active app found");
    return nullptr;
}

QJsonObject AppUsageRepository::getAppUsageSummary(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting app usage summary for session: %1").arg(sessionId.toString()));

    QJsonObject summary;

    if (!isInitialized()) {
        LOG_ERROR("Cannot get app usage summary: Repository not initialized");
        return summary;
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT "
        "COUNT(DISTINCT app_id) as unique_apps, "
        "COALESCE(SUM(EXTRACT(EPOCH FROM (COALESCE(end_time, CURRENT_TIMESTAMP) - start_time))), 0) as total_seconds "
        "FROM app_usage "
        "WHERE session_id = :session_id";

    logQueryWithValues(query, params);
    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> AppUsageModel* {
            // Create a temporary model to hold our statistics
            AppUsageModel* model = ModelFactory::createDefaultAppUsage();

            // Store the statistics in a JSON document
            QJsonObject stats;
            stats["unique_apps"] = query.value("unique_apps").toInt();
            stats["total_app_seconds"] = query.value("total_seconds").toDouble();

            // Set a custom property to store our stats
            model->setWindowTitle(QString::fromUtf8(QJsonDocument(stats).toJson()));

            return model;
        }
    );

    if (result) {
        // Extract the JSON we stored in the window title
        QJsonDocument doc = QJsonDocument::fromJson((*result)->windowTitle().toUtf8());
        if (doc.isObject()) {
            summary = doc.object();
        } else {
            summary["unique_apps"] = 0;
            summary["total_app_seconds"] = 0.0;
        }

        delete *result; // Clean up
    } else {
        summary["unique_apps"] = 0;
        summary["total_app_seconds"] = 0.0;
    }

    // Get current active app
    QSharedPointer<AppUsageModel> currentApp = getCurrentActiveApp(sessionId);
    summary["has_active_app"] = !currentApp.isNull();

    if (currentApp) {
        QJsonObject activeApp;
        activeApp["id"] = currentApp->id().toString(QUuid::WithoutBraces);
        activeApp["app_id"] = currentApp->appId().toString(QUuid::WithoutBraces);
        activeApp["start_time"] = currentApp->startTime().toString(Qt::ISODate);
        activeApp["window_title"] = currentApp->windowTitle();
        activeApp["duration_seconds"] = currentApp->duration();
        summary["active_app"] = activeApp;
    }

    LOG_INFO(QString("App usage summary retrieved for session %1").arg(sessionId.toString()));
    return summary;
}

QJsonArray AppUsageRepository::getTopApps(const QUuid &sessionId, int limit)
{
    LOG_DEBUG(QString("Getting top %1 apps for session: %2").arg(limit).arg(sessionId.toString()));

    QJsonArray topApps;

    if (!isInitialized()) {
        LOG_ERROR("Cannot get top apps: Repository not initialized");
        return topApps;
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);
    params["limit"] = QString::number(limit);

    QString query =
        "SELECT "
        "app_id, "
        "COUNT(*) as usage_count, "
        "COALESCE(SUM(EXTRACT(EPOCH FROM (COALESCE(end_time, CURRENT_TIMESTAMP) - start_time))), 0) as total_seconds "
        "FROM app_usage "
        "WHERE session_id = :session_id "
        "GROUP BY app_id "
        "ORDER BY total_seconds DESC "
        "LIMIT :limit";

    // Here we'll use a similar approach to getAppUsageSummary but with multiple rows
    auto appList = m_dbService->executeSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> AppUsageModel* {
            // Create a temporary model to hold our statistics
            AppUsageModel* model = ModelFactory::createDefaultAppUsage();

            // Populate the model with data from the query
            model->setAppId(QUuid(query.value("app_id").toString()));

            // Store usage count and total seconds in a JSON document in the window title
            QJsonObject stats;
            stats["usage_count"] = query.value("usage_count").toInt();
            stats["total_seconds"] = query.value("total_seconds").toDouble();

            model->setWindowTitle(QString::fromUtf8(QJsonDocument(stats).toJson()));

            return model;
        }
    );

    // Process all the results
    for (auto app : appList) {
        QJsonObject appObject;

        // Use the data we stored in the model
        appObject["app_id"] = app->appId().toString(QUuid::WithoutBraces);

        // Extract the statistics from the JSON we stored in window title
        QJsonDocument doc = QJsonDocument::fromJson(app->windowTitle().toUtf8());
        if (doc.isObject()) {
            QJsonObject stats = doc.object();
            appObject["usage_count"] = stats["usage_count"];
            appObject["total_seconds"] = stats["total_seconds"];
        } else {
            appObject["usage_count"] = 0;
            appObject["total_seconds"] = 0.0;
        }

        topApps.append(appObject);

        delete app; // Clean up
    }

    LOG_INFO(QString("Retrieved top %1 apps for session %2").arg(topApps.size()).arg(sessionId.toString()));
    return topApps;
}

void AppUsageRepository::logQueryWithValues(const QString& query, const QMap<QString, QVariant>& params)
{
    LOG_DEBUG("Executing query: " + query);

    if (!params.isEmpty()) {
        LOG_DEBUG("Query parameters:");
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            QString paramValue;

            if (it.value().isNull() || !it.value().isValid()) {
                paramValue = "NULL";
            } else if (it.value().type() == QVariant::String) {
                paramValue = "'" + it.value().toString() + "'";
            } else {
                paramValue = it.value().toString();
            }

            LOG_DEBUG(QString("  %1 = %2").arg(it.key(), paramValue));
        }
    }

    // For complex debugging, construct the actual query with values
    QString resolvedQuery = query;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        QString placeholder = ":" + it.key();
        QString value;

        if (it.value().isNull() || !it.value().isValid()) {
            value = "NULL";
        } else if (it.value().type() == QVariant::String) {
            value = "'" + it.value().toString() + "'";
        } else {
            value = it.value().toString();
        }

        resolvedQuery.replace(placeholder, value);
    }

    LOG_DEBUG("Resolved query: " + resolvedQuery);
}
