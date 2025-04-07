#include "SystemMetricsRepository.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include "Core/ModelFactory.h"

SystemMetricsRepository::SystemMetricsRepository(QObject *parent)
    : BaseRepository<SystemMetricsModel>(parent)
{
    LOG_DEBUG("SystemMetricsRepository created");
}

QString SystemMetricsRepository::getEntityName() const
{
    return "SystemMetrics";
}

QString SystemMetricsRepository::getTableName() const
{
    return "system_metrics";
}

QString SystemMetricsRepository::getIdParamName() const
{
    return "id";
}

QString SystemMetricsRepository::getModelId(SystemMetricsModel* model) const
{
    return model->id().toString();
}

QString SystemMetricsRepository::buildSaveQuery()
{
    return "INSERT INTO system_metrics "
           "(session_id, cpu_usage, gpu_usage, memory_usage, measurement_time, "
           "created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:session_id, :cpu_usage, :gpu_usage, :memory_usage, :measurement_time, "
           ":created_at, :created_by, :updated_at, :updated_by) "
           "RETURNING id";
}

QString SystemMetricsRepository::buildUpdateQuery()
{
    return "UPDATE system_metrics SET "
           "session_id = :session_id, "
           "cpu_usage = :cpu_usage, "
           "gpu_usage = :gpu_usage, "
           "memory_usage = :memory_usage, "
           "measurement_time = :measurement_time, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE metric_id = :metric_id";
}

QString SystemMetricsRepository::buildGetByIdQuery()
{
    return "SELECT * FROM system_metrics WHERE metric_id = :metric_id";
}

QString SystemMetricsRepository::buildGetAllQuery()
{
    return "SELECT * FROM system_metrics ORDER BY measurement_time DESC";
}

QString SystemMetricsRepository::buildRemoveQuery()
{
    return "DELETE FROM system_metrics WHERE metric_id = :metric_id";
}

QMap<QString, QVariant> SystemMetricsRepository::prepareParamsForSave(SystemMetricsModel* metrics)
{
    QMap<QString, QVariant> params;
    
    params["session_id"] = metrics->sessionId().toString(QUuid::WithoutBraces);
    params["cpu_usage"] = QString::number(metrics->cpuUsage());
    params["gpu_usage"] = QString::number(metrics->gpuUsage());
    params["memory_usage"] = QString::number(metrics->memoryUsage());
    params["measurement_time"] = metrics->measurementTime().toUTC();
    params["created_at"] = metrics->createdAt().toUTC();
    params["created_by"] = metrics->createdBy().isNull() ? QVariant(QVariant::Invalid) : metrics->createdBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = metrics->updatedAt().toUTC();
    params["updated_by"] = metrics->updatedBy().isNull() ? QVariant(QVariant::Invalid) : metrics->updatedBy().toString(QUuid::WithoutBraces);
    
    return params;
}

QMap<QString, QVariant> SystemMetricsRepository::prepareParamsForUpdate(SystemMetricsModel* metrics)
{
    QMap<QString, QVariant> params = prepareParamsForSave(metrics);
    params["metric_id"] = metrics->id().toString(QUuid::WithoutBraces);
    
    return params;
}

SystemMetricsModel* SystemMetricsRepository::createModelFromQuery(const QSqlQuery &query)
{
    // Use ModelFactory to create the model
    return ModelFactory::createSystemMetricsFromQuery(query);
}

bool SystemMetricsRepository::validateModel(SystemMetricsModel* model, QStringList& errors)
{
    return ModelFactory::validateSystemMetricsModel(model, errors);
}

QList<QSharedPointer<SystemMetricsModel>> SystemMetricsRepository::getBySessionId(
    const QUuid &sessionId,
    int limit,
    int offset)
{
    LOG_DEBUG(QString("Getting system metrics by session ID: %1 (limit: %2, offset: %3)")
              .arg(sessionId.toString())
              .arg(limit)
              .arg(offset));

    if (!isInitialized()) {
        LOG_ERROR(QString("Cannot get system metrics by session ID: Repository not initialized"));
        return QList<QSharedPointer<SystemMetricsModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM system_metrics WHERE session_id = :session_id ORDER BY measurement_time DESC";

    // Add limit and offset if provided
    if (limit > 0) {
        query += QString(" LIMIT %1").arg(limit);

        if (offset > 0) {
            query += QString(" OFFSET %1").arg(offset);
        }
    }

    QVector<SystemMetricsModel*> metrics = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> SystemMetricsModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<SystemMetricsModel>> result;
    for (auto metric : metrics) {
        result.append(QSharedPointer<SystemMetricsModel>(metric));
    }

    LOG_INFO(QString("Retrieved %1 system metrics for session %2 (limit: %3, offset: %4)")
             .arg(result.size())
             .arg(sessionId.toString())
             .arg(limit)
             .arg(offset));
    return result;
}

QList<QSharedPointer<SystemMetricsModel>> SystemMetricsRepository::getByTimeRange(
    const QUuid &sessionId,
    const QDateTime &startTime,
    const QDateTime &endTime,
    int limit,
    int offset)
{
    LOG_DEBUG(QString("Getting system metrics by time range for session: %1 (limit: %2, offset: %3)")
              .arg(sessionId.toString())
              .arg(limit)
              .arg(offset));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get system metrics by time range: Repository not initialized");
        return QList<QSharedPointer<SystemMetricsModel>>();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM system_metrics WHERE session_id = :session_id";

    // Add time constraints if provided
    if (startTime.isValid()) {
        params["start_time"] = startTime.toUTC();
        query += " AND measurement_time >= :start_time";
    }

    if (endTime.isValid()) {
        params["end_time"] = endTime.toUTC();
        query += " AND measurement_time <= :end_time";
    }

    // Order by measurement time
    query += " ORDER BY measurement_time ASC";

    // Add limit and offset if provided
    if (limit > 0) {
        query += QString(" LIMIT %1").arg(limit);

        if (offset > 0) {
            query += QString(" OFFSET %1").arg(offset);
        }
    }

    QVector<SystemMetricsModel*> metrics = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> SystemMetricsModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<SystemMetricsModel>> result;
    for (auto metric : metrics) {
        result.append(QSharedPointer<SystemMetricsModel>(metric));
    }

    // Log details about the query
    QString timeRangeInfo;
    if (startTime.isValid() && endTime.isValid()) {
        timeRangeInfo = QString("from %1 to %2").arg(startTime.toUTC().toString(), endTime.toUTC().toString());
    } else if (startTime.isValid()) {
        timeRangeInfo = QString("from %1 onwards").arg(startTime.toUTC().toString());
    } else if (endTime.isValid()) {
        timeRangeInfo = QString("until %1").arg(endTime.toUTC().toString());
    } else {
        timeRangeInfo = "for all time";
    }

    LOG_INFO(QString("Retrieved %1 system metrics %2 for session %3 (limit: %4, offset: %5)")
             .arg(result.size())
             .arg(timeRangeInfo)
             .arg(sessionId.toString())
             .arg(limit)
             .arg(offset));

    return result;
}

QJsonObject SystemMetricsRepository::getAverageMetrics(const QUuid &sessionId)
{
    LOG_DEBUG(QString("Getting average metrics for session: %1").arg(sessionId.toString()));

    if (!isInitialized()) {
        LOG_ERROR(QString("Cannot get average metrics: Repository not initialized").arg(sessionId.toString()));
        return QJsonObject();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT "
        "AVG(cpu_usage) as avg_cpu_usage, "
        "AVG(gpu_usage) as avg_gpu_usage, "
        "AVG(memory_usage) as avg_memory_usage, "
        "COUNT(*) as sample_count, "
        "MIN(measurement_time) as start_time, "
        "MAX(measurement_time) as end_time "
        "FROM system_metrics "
        "WHERE session_id = :session_id";

    QJsonObject result;

    auto metrics = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [&result, sessionId](const QSqlQuery& query) -> SystemMetricsModel* {
            if (query.isValid()) {
                result["avg_cpu_usage"] = query.value("avg_cpu_usage").toDouble();
                result["avg_gpu_usage"] = query.value("avg_gpu_usage").toDouble();
                result["avg_memory_usage"] = query.value("avg_memory_usage").toDouble();
                result["sample_count"] = query.value("sample_count").toInt();
                result["start_time"] = query.value("start_time").toDateTime().toUTC().toString();
                result["end_time"] = query.value("end_time").toDateTime().toUTC().toString();
            }
            
            // Create a default metrics model - we're not actually using this for the returned data
            // but it's required by the API
            return ModelFactory::createDefaultSystemMetrics(sessionId);
        }
    );

    if (metrics) {
        delete *metrics; // Clean up the model since we're not returning it
    }

    LOG_INFO(QString("Average metrics retrieved for session %1").arg(sessionId.toString()));
    return result;
}

QJsonArray SystemMetricsRepository::getMetricsTimeSeries(const QUuid &sessionId, const QString &metricType)
{
    LOG_DEBUG(QString("Getting metrics time series for session: %1, metric: %2").arg(sessionId.toString()).arg(metricType));

    if (!isInitialized()) {
        LOG_ERROR(QString("Cannot get metrics time series: Repository not initialized").arg(sessionId.toString()));
        return QJsonArray();
    }

    // Validate metric type
    if (metricType != "cpu_usage" && metricType != "gpu_usage" && metricType != "memory_usage") {
        LOG_WARNING(QString("Invalid metric type: %1").arg(metricType));
        return QJsonArray();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);

    QString query = QString(
        "SELECT measurement_time, %1 as value "
        "FROM system_metrics "
        "WHERE session_id = :session_id "
        "ORDER BY measurement_time ASC"
    ).arg(metricType);

    QJsonArray result;

    auto metrics = m_dbService->executeSelectQuery(
        query,
        params,
        [&result](const QSqlQuery& query) -> SystemMetricsModel* {
            if (query.isValid()) {
                QJsonObject dataPoint;
                dataPoint["time"] = query.value("measurement_time").toDateTime().toUTC().toString();
                dataPoint["value"] = query.value("value").toDouble();
                result.append(dataPoint);
            }
            // Create a default model to satisfy API requirements - not actually used
            return ModelFactory::createDefaultSystemMetrics();
        }
    );

    // Clean up all the dummy models created
    for (auto model : metrics) {
        delete model;
    }

    LOG_INFO(QString("Time series data retrieved for session %1, metric %2").arg(sessionId.toString()).arg(metricType));
    return result;
}

QJsonArray SystemMetricsRepository::getMetricsTimeSeries(const QUuid &sessionId, const QString &metricType, int limit)
{
    LOG_DEBUG(QString("Getting metrics time series for session: %1, metric: %2, limit: %3")
             .arg(sessionId.toString())
             .arg(metricType)
             .arg(limit));

    if (!isInitialized()) {
        LOG_ERROR(QString("Cannot get metrics time series: Repository not initialized").arg(sessionId.toString()));
        return QJsonArray();
    }

    // Validate metric type
    if (metricType != "cpu_usage" && metricType != "gpu_usage" && metricType != "memory_usage") {
        LOG_WARNING(QString("Invalid metric type: %1").arg(metricType));
        return QJsonArray();
    }

    QMap<QString, QVariant> params;
    params["session_id"] = sessionId.toString(QUuid::WithoutBraces);
    
    QString query = QString(
        "SELECT measurement_time, %1 as value "
        "FROM system_metrics "
        "WHERE session_id = :session_id "
        "ORDER BY measurement_time ASC"
    ).arg(metricType);
    
    if (limit > 0) {
        query += QString(" LIMIT %1").arg(limit);
    }

    QJsonArray result;

    auto metrics = m_dbService->executeSelectQuery(
        query,
        params,
        [&result](const QSqlQuery& query) -> SystemMetricsModel* {
            if (query.isValid()) {
                QJsonObject dataPoint;
                dataPoint["time"] = query.value("measurement_time").toDateTime().toUTC().toString();
                dataPoint["value"] = query.value("value").toDouble();
                result.append(dataPoint);
            }
            // Create a default model to satisfy API requirements - not actually used
            return ModelFactory::createDefaultSystemMetrics();
        }
    );

    // Clean up all the dummy models created
    for (auto model : metrics) {
        delete model;
    }

    LOG_INFO(QString("Time series data retrieved for session %1, metric %2 (limit: %3)")
            .arg(sessionId.toString())
            .arg(metricType)
            .arg(limit));
    return result;
}

