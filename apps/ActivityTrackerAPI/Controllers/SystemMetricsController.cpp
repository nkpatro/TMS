#include "SystemMetricsController.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QUrlQuery>
#include <algorithm>
#include "../Utils/SystemInfo.h"
#include "logger/logger.h"
#include "httpserver/response.h"

SystemMetricsController::SystemMetricsController(QObject *parent)
    : ApiControllerBase(parent)
    , m_systemMetricsRepository(nullptr)
    , m_initialized(false)
{
    LOG_DEBUG("SystemMetricsController created");
}

SystemMetricsController::SystemMetricsController(SystemMetricsRepository* repository, QObject *parent)
    : ApiControllerBase(parent)
    , m_systemMetricsRepository(repository)
    , m_initialized(false)
{
    LOG_DEBUG("SystemMetricsController created with existing repository");

    // If a repository was provided, check if it's initialized
    if (m_systemMetricsRepository && m_systemMetricsRepository->isInitialized()) {
        m_initialized = true;
        LOG_INFO("SystemMetricsController initialized successfully");
    }
}

SystemMetricsController::~SystemMetricsController()
{
    // Only delete repository if we created it
    if (m_systemMetricsRepository && m_systemMetricsRepository->parent() == this) {
        delete m_systemMetricsRepository;
        m_systemMetricsRepository = nullptr;
    }

    LOG_DEBUG("SystemMetricsController destroyed");
}

bool SystemMetricsController::initialize()
{
    if (m_initialized) {
        LOG_WARNING("SystemMetricsController already initialized");
        return true;
    }

    LOG_DEBUG("Initializing SystemMetricsController");

    try {
        // Check if repository is valid and initialized
        if (!m_systemMetricsRepository) {
            LOG_ERROR("SystemMetrics repository not provided");
            return false;
        }

        if (!m_systemMetricsRepository->isInitialized()) {
            LOG_ERROR("SystemMetrics repository not initialized");
            return false;
        }

        m_initialized = true;
        LOG_INFO("SystemMetricsController initialized successfully");
        return true;
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception during SystemMetricsController initialization: %1").arg(e.what()));
        return false;
    }
}

void SystemMetricsController::setupRoutes(QHttpServer &server)
{
    if (!m_initialized) {
        LOG_ERROR("Cannot setup routes - SystemMetricsController not initialized");
        return;
    }

    LOG_INFO("Setting up SystemMetricsController routes");

    // Get all metrics
    server.route("/api/metrics", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetMetrics(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get metrics by session ID
    server.route("/api/sessions/<arg>/metrics", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetMetricsBySessionId(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Record metrics (without session ID)
    server.route("/api/metrics", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleRecordMetrics(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Record metrics for a specific session
    server.route("/api/sessions/<arg>/metrics", QHttpServerRequest::Method::Post,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleRecordMetricsForSession(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get average metrics for a session
    server.route("/api/sessions/<arg>/metrics/average", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetAverageMetrics(sessionId, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get metrics time series for a session
    server.route("/api/sessions/<arg>/metrics/timeseries/<arg>", QHttpServerRequest::Method::Get,
        [this](const qint64 sessionId, const QString &metricType, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetMetricsTimeSeries(sessionId, metricType, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get current system information
    server.route("/api/system/info", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetSystemInfo(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    LOG_INFO("SystemMetricsController routes configured");
}

QHttpServerResponse SystemMetricsController::handleGetMetrics(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SystemMetricsController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET all metrics request");

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

        // Get the most recent metrics
        QList<QSharedPointer<SystemMetricsModel>> metrics = m_systemMetricsRepository->getAll();

        // Limit the results
        if (metrics.size() > limit) {
            metrics = metrics.mid(0, limit);
        }

        QJsonArray metricsArray;
        for (const auto &metric : metrics) {
            metricsArray.append(systemMetricsToJson(metric.data()));
        }

        return createSuccessResponse(metricsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting metrics: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve metrics: %1").arg(e.what()));
    }
}

QHttpServerResponse SystemMetricsController::handleGetMetricsBySessionId(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SystemMetricsController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET metrics by session ID request: %1").arg(sessionId));

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

        if (limit <= 0 || limit > 1000) {
            limit = 100; // Default limit
        }

        // Since getBySessionId with limit and offset parameters doesn't exist,
        // we'll use the getAll method and filter manually
        QList<QSharedPointer<SystemMetricsModel>> allMetrics = m_systemMetricsRepository->getAll();

        // Filter by session ID
        QList<QSharedPointer<SystemMetricsModel>> sessionMetrics;
        for (const auto &metric : allMetrics) {
            if (metric->sessionId() == sessionUuid) {
                sessionMetrics.append(metric);
            }
        }

        // Sort by measurement time (newest first)
        std::sort(sessionMetrics.begin(), sessionMetrics.end(),
            [](const QSharedPointer<SystemMetricsModel> &a, const QSharedPointer<SystemMetricsModel> &b) {
                return a->measurementTime() > b->measurementTime();
            });

        // Apply limit and offset
        if (offset >= sessionMetrics.size()) {
            sessionMetrics.clear();
        } else {
            int endIndex = qMin(offset + limit, sessionMetrics.size());
            sessionMetrics = sessionMetrics.mid(offset, endIndex - offset);
        }

        QJsonArray metricsArray;
        for (const auto &metric : sessionMetrics) {
            metricsArray.append(systemMetricsToJson(metric.data()));
        }

        LOG_INFO(QString("Retrieved %1 metrics for session %2").arg(sessionMetrics.size()).arg(sessionId));
        return createSuccessResponse(metricsArray);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting metrics by session ID: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve metrics: %1").arg(e.what()));
    }
}

QHttpServerResponse SystemMetricsController::handleRecordMetrics(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SystemMetricsController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing RECORD metrics request");

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

        // Check if session ID is provided
        if (!json.contains("session_id") || json["session_id"].toString().isEmpty()) {
            LOG_WARNING("Session ID is required");
            return createErrorResponse("Session ID is required", QHttpServerResponder::StatusCode::BadRequest);
        }

        QUuid sessionId = QUuid(json["session_id"].toString());

        // Create new system metrics
        SystemMetricsModel *metrics = new SystemMetricsModel();
        metrics->setSessionId(sessionId);

        // Set actual metrics values
        if (json.contains("cpu_usage") && json["cpu_usage"].isDouble()) {
            metrics->setCpuUsage(json["cpu_usage"].toDouble());
        } else {
            // Use current CPU usage from system info
            metrics->setCpuUsage(SystemInfo::getCurrentCPUUsage());
        }

        if (json.contains("gpu_usage") && json["gpu_usage"].isDouble()) {
            metrics->setGpuUsage(json["gpu_usage"].toDouble());
        } else {
            // Use current GPU usage from system info
            metrics->setGpuUsage(SystemInfo::getCurrentGPUUsage());
        }

        if (json.contains("memory_usage") && json["memory_usage"].isDouble()) {
            metrics->setMemoryUsage(json["memory_usage"].toDouble());
        } else {
            // Use current memory usage from system info
            metrics->setMemoryUsage(SystemInfo::getCurrentMemoryUsage());
        }

        // Set measurement time
        if (json.contains("measurement_time") && !json["measurement_time"].toString().isEmpty()) {
            metrics->setMeasurementTime(QDateTime::fromString(json["measurement_time"].toString(), Qt::ISODate));
        } else {
            metrics->setMeasurementTime(QDateTime::currentDateTimeUtc());
        }

        // Set metadata
        QUuid userId = QUuid(userData["id"].toString());
        metrics->setCreatedBy(userId);
        metrics->setUpdatedBy(userId);

        bool success = m_systemMetricsRepository->save(metrics);

        if (!success) {
            delete metrics;
            LOG_ERROR("Failed to record system metrics");
            return createErrorResponse("Failed to record system metrics", QHttpServerResponder::StatusCode::InternalServerError);
        }

        QJsonObject response = systemMetricsToJson(metrics);
        LOG_INFO(QString("System metrics recorded successfully: %1").arg(metrics->id().toString()));
        delete metrics;

        return createSuccessResponse(response, QHttpServerResponder::StatusCode::Created);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception recording metrics: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to record metrics: %1").arg(e.what()));
    }
}

QHttpServerResponse SystemMetricsController::handleGetSystemInfo(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SystemMetricsController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET system info request");

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        // Get current system information
        QJsonObject systemInfo = SystemInfo::getAllSystemInfo();

        return createSuccessResponse(systemInfo);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting system info: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to get system info: %1").arg(e.what()));
    }
}

QHttpServerResponse SystemMetricsController::handleRecordMetricsForSession(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SystemMetricsController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing RECORD metrics for session ID: %1").arg(sessionId));

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

        // Create new system metrics
        SystemMetricsModel *metrics = new SystemMetricsModel();
        metrics->setSessionId(sessionUuid);

        // Set actual metrics values
        if (json.contains("cpu_usage") && json["cpu_usage"].isDouble()) {
            metrics->setCpuUsage(json["cpu_usage"].toDouble());
        } else {
            // Use current CPU usage from system info
            metrics->setCpuUsage(SystemInfo::getCurrentCPUUsage());
        }

        if (json.contains("gpu_usage") && json["gpu_usage"].isDouble()) {
            metrics->setGpuUsage(json["gpu_usage"].toDouble());
        } else {
            // Use current GPU usage from system info
            metrics->setGpuUsage(SystemInfo::getCurrentGPUUsage());
        }

        if (json.contains("memory_usage") && json["memory_usage"].isDouble()) {
            metrics->setMemoryUsage(json["memory_usage"].toDouble());
        } else {
            // Use current memory usage from system info
            metrics->setMemoryUsage(SystemInfo::getCurrentMemoryUsage());
        }

        // Set measurement time
        if (json.contains("measurement_time") && !json["measurement_time"].toString().isEmpty()) {
            metrics->setMeasurementTime(QDateTime::fromString(json["measurement_time"].toString(), Qt::ISODate));
        } else {
            metrics->setMeasurementTime(QDateTime::currentDateTimeUtc());
        }

        // Set metadata
        QUuid userId = QUuid(userData["id"].toString());
        metrics->setCreatedBy(userId);
        metrics->setUpdatedBy(userId);

        bool success = m_systemMetricsRepository->save(metrics);

        if (!success) {
            delete metrics;
            LOG_ERROR("Failed to record system metrics");
            return createErrorResponse("Failed to record system metrics", QHttpServerResponder::StatusCode::InternalServerError);
        }

        QJsonObject response = systemMetricsToJson(metrics);
        LOG_INFO(QString("System metrics recorded successfully for session %1: %2")
                .arg(sessionId).arg(metrics->id().toString()));
        delete metrics;

        return createSuccessResponse(response, QHttpServerResponder::StatusCode::Created);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception recording metrics for session: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to record metrics: %1").arg(e.what()));
    }
}

QHttpServerResponse SystemMetricsController::handleGetAverageMetrics(const qint64 sessionId, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SystemMetricsController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET average metrics for session ID: %1").arg(sessionId));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Since getAverageMetrics doesn't exist, we'll calculate them manually
        QList<QSharedPointer<SystemMetricsModel>> allMetrics = m_systemMetricsRepository->getAll();

        // Filter by session ID
        QList<QSharedPointer<SystemMetricsModel>> sessionMetrics;
        for (const auto &metric : allMetrics) {
            if (metric->sessionId() == sessionUuid) {
                sessionMetrics.append(metric);
            }
        }

        if (sessionMetrics.isEmpty()) {
            LOG_WARNING(QString("No metrics found for session ID: %1").arg(sessionId));
            return createSuccessResponse(QJsonObject{{"message", "No metrics found for this session"}});
        }

        // Calculate averages
        double totalCpuUsage = 0.0;
        double totalGpuUsage = 0.0;
        double totalMemoryUsage = 0.0;
        int count = sessionMetrics.size();

        for (const auto &metric : sessionMetrics) {
            totalCpuUsage += metric->cpuUsage();
            totalGpuUsage += metric->gpuUsage();
            totalMemoryUsage += metric->memoryUsage();
        }

        QJsonObject averageMetrics;
        averageMetrics["session_id"] = uuidToString(sessionUuid);
        averageMetrics["avg_cpu_usage"] = totalCpuUsage / count;
        averageMetrics["avg_gpu_usage"] = totalGpuUsage / count;
        averageMetrics["avg_memory_usage"] = totalMemoryUsage / count;
        averageMetrics["sample_count"] = count;

        // Add time range information
        if (!sessionMetrics.isEmpty()) {
            QDateTime earliestTime = sessionMetrics.first()->measurementTime();
            QDateTime latestTime = sessionMetrics.first()->measurementTime();

            for (const auto &metric : sessionMetrics) {
                if (metric->measurementTime() < earliestTime) {
                    earliestTime = metric->measurementTime();
                }
                if (metric->measurementTime() > latestTime) {
                    latestTime = metric->measurementTime();
                }
            }

            averageMetrics["start_time"] = earliestTime.toString(Qt::ISODate);
            averageMetrics["end_time"] = latestTime.toString(Qt::ISODate);
        }

        LOG_INFO(QString("Average metrics calculated from %1 samples for session %2").arg(count).arg(sessionId));
        return createSuccessResponse(averageMetrics);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting average metrics: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to get average metrics: %1").arg(e.what()));
    }
}

QHttpServerResponse SystemMetricsController::handleGetMetricsTimeSeries(const qint64 sessionId, const QString &metricType, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("SystemMetricsController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET metrics time series for session ID: %1, metric type: %2").arg(sessionId).arg(metricType));

    // Check authentication
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized request");
        return Http::Response::unauthorized("Unauthorized");
    }

    try {
        QUuid sessionUuid = stringToUuid(QString::number(sessionId));

        // Validate metric type
        if (metricType != "cpu" && metricType != "gpu" && metricType != "memory" && metricType != "all") {
            LOG_WARNING(QString("Invalid metric type: %1").arg(metricType));
            return createErrorResponse(QString("Invalid metric type. Must be one of: cpu, gpu, memory, all"), QHttpServerResponder::StatusCode::BadRequest);
        }

        // Since getMetricsTimeSeries doesn't exist, we'll generate the time series manually
        QList<QSharedPointer<SystemMetricsModel>> allMetrics = m_systemMetricsRepository->getAll();

        // Filter by session ID
        QList<QSharedPointer<SystemMetricsModel>> sessionMetrics;
        for (const auto &metric : allMetrics) {
            if (metric->sessionId() == sessionUuid) {
                sessionMetrics.append(metric);
            }
        }

        // Sort by measurement time
        std::sort(sessionMetrics.begin(), sessionMetrics.end(),
            [](const QSharedPointer<SystemMetricsModel> &a, const QSharedPointer<SystemMetricsModel> &b) {
                return a->measurementTime() < b->measurementTime();
            });

        QJsonObject timeSeriesData;
        timeSeriesData["session_id"] = uuidToString(sessionUuid);
        timeSeriesData["metric_type"] = metricType;

        // Build the time series data arrays
        if (metricType == "cpu" || metricType == "all") {
            QJsonArray cpuSeries;
            for (const auto &metric : sessionMetrics) {
                QJsonObject dataPoint;
                dataPoint["time"] = metric->measurementTime().toString(Qt::ISODate);
                dataPoint["value"] = metric->cpuUsage();
                cpuSeries.append(dataPoint);
            }
            timeSeriesData["cpu_usage"] = cpuSeries;
        }

        if (metricType == "gpu" || metricType == "all") {
            QJsonArray gpuSeries;
            for (const auto &metric : sessionMetrics) {
                QJsonObject dataPoint;
                dataPoint["time"] = metric->measurementTime().toString(Qt::ISODate);
                dataPoint["value"] = metric->gpuUsage();
                gpuSeries.append(dataPoint);
            }
            timeSeriesData["gpu_usage"] = gpuSeries;
        }

        if (metricType == "memory" || metricType == "all") {
            QJsonArray memorySeries;
            for (const auto &metric : sessionMetrics) {
                QJsonObject dataPoint;
                dataPoint["time"] = metric->measurementTime().toString(Qt::ISODate);
                dataPoint["value"] = metric->memoryUsage();
                memorySeries.append(dataPoint);
            }
            timeSeriesData["memory_usage"] = memorySeries;
        }

        LOG_INFO(QString("Time series data generated with %1 points for session %2, metric type %3")
                .arg(sessionMetrics.size()).arg(sessionId).arg(metricType));
        return createSuccessResponse(timeSeriesData);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting metrics time series: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to get metrics time series: %1").arg(e.what()));
    }
}

// Helper methods implementation
QJsonObject SystemMetricsController::systemMetricsToJson(SystemMetricsModel *metrics) const
{
    QJsonObject json;
    json["metric_id"] = uuidToString(metrics->id());
    json["session_id"] = uuidToString(metrics->sessionId());
    json["cpu_usage"] = metrics->cpuUsage();
    json["gpu_usage"] = metrics->gpuUsage();
    json["memory_usage"] = metrics->memoryUsage();
    json["measurement_time"] = metrics->measurementTime().toString(Qt::ISODate);
    json["created_at"] = metrics->createdAt().toString(Qt::ISODate);

    if (!metrics->createdBy().isNull()) {
        json["created_by"] = uuidToString(metrics->createdBy());
    }

    json["updated_at"] = metrics->updatedAt().toString(Qt::ISODate);

    if (!metrics->updatedBy().isNull()) {
        json["updated_by"] = uuidToString(metrics->updatedBy());
    }

    return json;
}

QJsonObject SystemMetricsController::extractJsonFromRequest(const QHttpServerRequest &request, bool &ok)
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

QUuid SystemMetricsController::stringToUuid(const QString &str) const
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

QString SystemMetricsController::uuidToString(const QUuid &uuid) const
{
    return uuid.toString(QUuid::WithoutBraces);
}

