#include "ServerStatusController.h"
#include "logger/logger.h"
#include "httpserver/response.h"
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QSysInfo>
#include <QHostInfo>

ServerStatusController::ServerStatusController(QObject *parent)
    : ApiControllerBase(parent)
    , m_startTime(QDateTime::currentDateTimeUtc())
    , m_version("1.0.0") // Replace with actual version from configuration
    , m_buildDate(__DATE__ " " __TIME__)
{
    LOG_DEBUG("ServerStatusController created");
}

ServerStatusController::~ServerStatusController()
{
    LOG_DEBUG("ServerStatusController destroyed");
}

void ServerStatusController::setupRoutes(QHttpServer &server)
{
    LOG_INFO("Setting up ServerStatusController routes");

    // Simple ping endpoint - returns pong
    server.route("/api/status/ping", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handlePingRequest(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Health check endpoint - returns detailed system health
    server.route("/api/status/health", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleHealthCheck(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Version information endpoint
    server.route("/api/status/version", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleVersionInfo(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    LOG_INFO("ServerStatusController routes configured");
}

QHttpServerResponse ServerStatusController::handlePingRequest(const QHttpServerRequest &request)
{
    // Simple ping-pong response - no authentication required
    QJsonObject response;
    response["status"] = "ok";
    response["message"] = "pong";
    response["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    return createSuccessResponse(response);
}

QHttpServerResponse ServerStatusController::handleHealthCheck(const QHttpServerRequest &request)
{
    // Check authentication - for security reasons, health checks may require auth
    QJsonObject userData;
    if (!isUserAuthorized(request, userData)) {
        LOG_WARNING("Unauthorized health check request");
        return Http::Response::unauthorized("Unauthorized");
    }

    // Calculate uptime
    QDateTime now = QDateTime::currentDateTimeUtc();
    qint64 uptimeSeconds = m_startTime.secsTo(now);

    // Get system info
    QJsonObject systemInfo;
    systemInfo["os"] = QSysInfo::prettyProductName();
    systemInfo["kernel_type"] = QSysInfo::kernelType();
    systemInfo["kernel_version"] = QSysInfo::kernelVersion();
    systemInfo["cpu_architecture"] = QSysInfo::currentCpuArchitecture();
    systemInfo["hostname"] = QHostInfo::localHostName();

    // Create response
    QJsonObject response;
    response["status"] = "ok";
    response["server_time"] = now.toString(Qt::ISODate);
    response["uptime_seconds"] = uptimeSeconds;
    response["system_info"] = systemInfo;
    response["version"] = m_version;
    response["build_date"] = m_buildDate;

    // Add memory usage if available
    #ifdef Q_OS_LINUX
    // On Linux we could add memory info from /proc/meminfo
    #endif

    return createSuccessResponse(response);
}

QHttpServerResponse ServerStatusController::handleVersionInfo(const QHttpServerRequest &request)
{
    // Version info is public - no authentication required
    QJsonObject response;
    response["version"] = m_version;
    response["build_date"] = m_buildDate;
    response["qt_version"] = qVersion();
    response["server_time"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    return createSuccessResponse(response);
}