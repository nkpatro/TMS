#include "APIManager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QUrlQuery>
#include "logger/logger.h"

APIManager::APIManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_initialized(false)
{
}

APIManager::~APIManager()
{
}

bool APIManager::initialize(const QString &serverUrl)
{
    LOG_INFO("Initializing APIManager with server URL: " + serverUrl);

    if (m_initialized) {
        LOG_WARNING("APIManager already initialized");
        return true;
    }

    m_serverUrl = serverUrl;

    // Ensure server URL ends with a slash
    if (!m_serverUrl.endsWith('/')) {
        m_serverUrl += '/';
    }

    m_initialized = true;
    return true;
}

bool APIManager::authenticate(const QString &username, const QString &machineId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_INFO(QString("Authenticating user: %1 on machine: %2").arg(username, machineId));

    // Store credentials for potential later reauthentication
    m_username = username;
    m_machineId = machineId;

    QJsonObject authData;
    authData["username"] = username;
    authData["machine_id"] = machineId;

    // Add service_id parameter - required by the server API
    // Using a default application ID for the service
    authData["service_id"] = "activity-tracker-service";

    bool success = sendRequest("auth/service-token", authData, responseData, "POST", false);

    if (success && responseData.contains("token")) {
        QMutexLocker locker(&m_mutex);
        m_authToken = responseData["token"].toString();
        LOG_INFO("Authentication successful, received service token");
        return true;
    }

    LOG_ERROR("Authentication failed");
    return false;
}

bool APIManager::findSessionForDate(const QJsonObject &query, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_DEBUG("Looking for session by date");

    // Create query string
    QUrlQuery urlQuery;
    if (query.contains("date")) urlQuery.addQueryItem("date", query["date"].toString());
    if (query.contains("username")) urlQuery.addQueryItem("username", query["username"].toString());
    if (query.contains("machine_id")) urlQuery.addQueryItem("machine_id", query["machine_id"].toString());

    QString endpoint = "sessions/search?" + urlQuery.toString();

    // This is a GET request
    bool success = sendRequest(endpoint, QJsonObject(), responseData, "GET");

    if (success && responseData.contains("session_id")) {
        LOG_DEBUG(QString("Found session for date: %1").arg(query["date"].toString()));
        return true;
    }

    LOG_DEBUG("No session found for the specified date");
    return false;
}

bool APIManager::createSession(const QJsonObject &sessionData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_INFO("Creating new session");

    return sendRequest("sessions", sessionData, responseData);
}

bool APIManager::getSession(const QUuid &sessionId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Fetching session: %1").arg(sessionId.toString()));

    QString endpoint = "sessions/" + sessionId.toString().remove('{').remove('}');

    // This is a GET request
    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::endSession(const QUuid &sessionId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_INFO(QString("Ending session: %1").arg(sessionId.toString()));

    QString endpoint = "sessions/" + sessionId.toString() + "/end";

    // Empty post data
    QJsonObject emptyData;

    return sendRequest(endpoint, emptyData, responseData);
}

bool APIManager::getLastSessionEvent(const QJsonObject &query, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString sessionId = query["session_id"].toString();
    QString eventType = query["event_type"].toString();

    LOG_DEBUG(QString("Fetching last %1 event for session: %2").arg(eventType, sessionId));

    // Create query string
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("event_type", eventType);
    urlQuery.addQueryItem("latest", "true");

    QString endpoint = "sessions/" + sessionId + "/events?" + urlQuery.toString();

    // This is a GET request
    bool success = sendRequest(endpoint, QJsonObject(), responseData, "GET");

    // The response might be an array with one item
    if (success && responseData.contains("events") && responseData["events"].isArray()) {
        QJsonArray events = responseData["events"].toArray();
        if (events.size() > 0) {
            responseData = events.first().toObject();
            return true;
        }
        return false;  // No events found
    }

    return success;
}

bool APIManager::getLastEvent(const QJsonObject &query, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString sessionId = query["session_id"].toString();

    LOG_DEBUG(QString("Fetching last event for session: %1").arg(sessionId));

    // Create query string
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("latest", "true");
    urlQuery.addQueryItem("limit", "1");

    QString endpoint = "sessions/" + sessionId + "/activities?" + urlQuery.toString();

    // This is a GET request
    bool success = sendRequest(endpoint, QJsonObject(), responseData, "GET");

    // The response might be an array with one item
    if (success && responseData.contains("events") && responseData["events"].isArray()) {
        QJsonArray events = responseData["events"].toArray();
        if (events.size() > 0) {
            responseData = events.first().toObject();
            return true;
        }
        return false;  // No events found
    }

    return success;
}

bool APIManager::batchSessionEvents(const QJsonObject &eventsData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString sessionId = eventsData["session_id"].toString();

    LOG_DEBUG(QString("Sending batch session events for session: %1").arg(sessionId));

    QString endpoint = "sessions/" + sessionId + "/batch";

    // Create batch data
    QJsonObject batchData;
    batchData["session_events"] = eventsData["session_events"];

    QJsonObject responseData;
    return sendRequest(endpoint, batchData, responseData);
}

bool APIManager::batchActivityEvents(const QJsonObject &eventsData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString sessionId = eventsData["session_id"].toString();

    LOG_DEBUG(QString("Sending batch activity events for session: %1").arg(sessionId));

    QString endpoint = "sessions/" + sessionId + "/batch";

    // Create batch data
    QJsonObject batchData;
    batchData["activity_events"] = eventsData["activity_events"];

    QJsonObject responseData;
    return sendRequest(endpoint, batchData, responseData);
}

bool APIManager::startAppUsage(const QJsonObject &usageData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    // Check if session_id is present in the data
    if (!usageData.contains("session_id")) {
        LOG_ERROR("Cannot start app usage: session_id is missing");
        responseData["error"] = true;
        responseData["message"] = "Session ID is required";
        return false;
    }

    // Make sure app_id is present and valid
    if (!usageData.contains("app_id") || usageData["app_id"].toString().isEmpty() ||
        usageData["app_id"].toString() == "00000000-0000-0000-0000-000000000000") {
        LOG_ERROR("Cannot start app usage: valid app_id is required");
        responseData["error"] = true;
        responseData["message"] = "Valid application ID is required";
        return false;
        }

    // Get the session ID for constructing the correct endpoint
    QString sessionId = usageData["session_id"].toString().remove('{').remove('}');
    LOG_DEBUG(QString("Starting app usage tracking for session %1").arg(sessionId));

    // Create a copy of the data to ensure we don't modify the original
    QJsonObject data = usageData;

    // First try the session-specific endpoint
    QString sessionEndpoint = "sessions/" + sessionId + "/app-usages";
    bool success = sendRequest(sessionEndpoint, data, responseData);

    // If that fails, try the direct endpoint
    if (!success) {
        LOG_WARNING("Session-specific app usage endpoint failed, trying direct endpoint");
        success = sendRequest("app-usages", data, responseData);
    }

    return success;
}

bool APIManager::endAppUsage(const QUuid &usageId, const QJsonObject &usageData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    // Check if session_id is present in the data
    if (!usageData.contains("session_id")) {
        LOG_ERROR("Cannot end app usage: session_id is missing");
        responseData["error"] = true;
        responseData["message"] = "Session ID is required";
        return false;
    }

    // Validate usage ID
    if (usageId.isNull() || usageId == QUuid("00000000-0000-0000-0000-000000000000")) {
        LOG_ERROR("Cannot end app usage: invalid usage ID");
        responseData["error"] = true;
        responseData["message"] = "Valid usage ID is required";
        return false;
    }

    // Get the session ID for constructing the correct endpoint
    QString sessionId = usageData["session_id"].toString().remove('{').remove('}');
    LOG_DEBUG(QString("Ending app usage: %1 for session %2").arg(usageId.toString(), sessionId));

    // Remove braces from usage ID for endpoint
    QString cleanUsageId = usageId.toString().remove('{').remove('}');

    // Ensure session_id doesn't have braces
    QJsonObject data = usageData;
    if (data.contains("session_id")) {
        data["session_id"] = sessionId;
    }

    // First try the session-specific endpoint
    QString sessionEndpoint = "sessions/" + sessionId + "/app-usages/" + cleanUsageId + "/end";
    bool success = sendRequest(sessionEndpoint, data, responseData);

    // If that fails, try the direct endpoint
    if (!success) {
        LOG_WARNING("Session-specific app usage end endpoint failed, trying direct endpoint");
        QString directEndpoint = "app-usages/" + cleanUsageId + "/end";
        success = sendRequest(directEndpoint, data, responseData);
    }

    return success;
}

bool APIManager::batchSystemMetrics(const QJsonObject &metricsData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString sessionId = metricsData["session_id"].toString();

    LOG_DEBUG(QString("Sending batch system metrics for session: %1").arg(sessionId));

    QString endpoint = "sessions/" + sessionId + "/batch";

    // Create batch data
    QJsonObject batchData;
    batchData["system_metrics"] = metricsData["system_metrics"];

    QJsonObject responseData;
    return sendRequest(endpoint, batchData, responseData);
}

bool APIManager::startAfkPeriod(const QJsonObject &afkData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString sessionId = afkData["session_id"].toString();

    LOG_DEBUG(QString("Starting AFK period for session: %1").arg(sessionId));

    QString endpoint = "sessions/" + sessionId + "/afk/start";

    return sendRequest(endpoint, afkData, responseData);
}

bool APIManager::endAfkPeriod(const QUuid &afkId, const QJsonObject &afkData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_DEBUG(QString("Ending AFK period: %1").arg(afkId.toString()));

    // Get the session ID from the data
    QString sessionId;
    if (afkData.contains("session_id")) {
        sessionId = afkData["session_id"].toString();
    } else {
        LOG_WARNING("Session ID not provided for ending AFK period");
        return false;
    }

    QString endpoint = "sessions/" + sessionId + "/afk/end";

    return sendRequest(endpoint, afkData, responseData);
}

bool APIManager::sendRequest(const QString &endpoint, const QJsonObject &data, QJsonObject &responseData,
                           const QString &method, bool requiresAuth)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    // Construct full URL
    QString url = m_serverUrl + "api/" + endpoint;

    // Create request
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Add authentication if required
    if (requiresAuth) {
        QString token = getAuthToken();
        if (token.isEmpty()) {
            LOG_WARNING("No authentication token available, attempting to reauthenticate");

            // Try to reauthenticate if we have credentials
            if (!m_username.isEmpty() && !m_machineId.isEmpty()) {
                QJsonObject authResponse;
                bool authenticated = authenticate(m_username, m_machineId, authResponse);
                if (!authenticated) {
                    LOG_ERROR("Reauthentication failed");
                    return false;
                }

                // Now get the new token
                token = getAuthToken();
                if (token.isEmpty()) {
                    LOG_ERROR("Still no authentication token after reauthentication");
                    return false;
                }

                LOG_INFO("Reauthentication successful, proceeding with request");
            } else {
                LOG_ERROR("Cannot reauthenticate: missing username or machineId");
                return false;
            }
        }
        request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    }

    // Create a QEventLoop to make the request synchronous
    QEventLoop loop;
    QNetworkReply *reply = nullptr;

    // Add request timeout
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    // Set reasonable timeout (10 seconds)
    const int requestTimeoutMs = 10000;

    // Log request details at debug level
    LOG_DEBUG(QString("Sending %1 request to: %2").arg(method, url));

    // Send the request based on the method
    if (method == "GET") {
        reply = m_networkManager->get(request);
    } else if (method == "POST") {
        QJsonDocument doc(data);
        QByteArray jsonData = doc.toJson();
        LOG_DEBUG(QString("POST data: %1").arg(QString::fromUtf8(jsonData)));
        reply = m_networkManager->post(request, jsonData);
    } else if (method == "PUT") {
        QJsonDocument doc(data);
        QByteArray jsonData = doc.toJson();
        LOG_DEBUG(QString("PUT data: %1").arg(QString::fromUtf8(jsonData)));
        reply = m_networkManager->put(request, jsonData);
    } else if (method == "DELETE") {
        reply = m_networkManager->deleteResource(request);
    } else {
        LOG_ERROR("Unsupported HTTP method: " + method);
        return false;
    }

    // Connect signals to handle the reply
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    // Start the timeout timer
    timeoutTimer.start(requestTimeoutMs);

    // Start the event loop - this will block until the request is finished or timeout
    loop.exec();

    // Check if timeout occurred
    if (timeoutTimer.isActive()) {
        // Timer is still active, meaning the request finished before timeout
        timeoutTimer.stop();
    } else {
        // Timer is not active, meaning timeout occurred
        if (reply) {
            reply->abort();
            reply->deleteLater();
            LOG_ERROR(QString("Request timeout for %1 %2").arg(method, url));
            return false;
        }
    }

    // Process the reply
    bool success = processReply(reply, responseData);

    // Handle token expiration or auth errors
    if (!success && requiresAuth) {
        QNetworkReply::NetworkError error = reply->error();
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check for auth errors (401 Unauthorized or 403 Forbidden)
        if (httpStatus == 401 || httpStatus == 403) {
            LOG_WARNING("Authentication error, attempting to refresh token");

            // Clean up the old reply
            reply->deleteLater();

            // Try to reauthenticate
            if (!m_username.isEmpty() && !m_machineId.isEmpty()) {
                QJsonObject authResponse;
                bool authenticated = authenticate(m_username, m_machineId, authResponse);
                if (authenticated) {
                    LOG_INFO("Token refreshed successfully, retrying request");

                    // Retry the request with the new token
                    return sendRequest(endpoint, data, responseData, method, requiresAuth);
                } else {
                    LOG_ERROR("Failed to refresh token");
                }
            }
        }
    }

    // Cleanup
    reply->deleteLater();
    return success;
}

bool APIManager::processReply(QNetworkReply *reply, QJsonObject &responseData)
{
    if (!reply) {
        LOG_ERROR("Network reply is null");
        m_lastErrorCode = 0;
        m_lastErrorMessage = "Null reply";
        return false;
    }

    // Get HTTP status code if available
    m_lastErrorCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // Check for network error
    QNetworkReply::NetworkError error = reply->error();
    if (error != QNetworkReply::NoError) {
        QString errorString = reply->errorString();
        QString requestUrl = reply->url().toString();

        // Log the error with detailed information
        LOG_ERROR(QString("Network error (%1): %2, HTTP status: %3, URL: %4")
                 .arg(error)
                 .arg(errorString)
                 .arg(m_lastErrorCode)
                 .arg(requestUrl));

        // Try to parse error response if available
        QByteArray errorResponse = reply->readAll();
        m_lastErrorMessage = errorString;

        if (!errorResponse.isEmpty()) {
            LOG_DEBUG(QString("Error response: %1").arg(QString::fromUtf8(errorResponse)));

            // Try to parse as JSON
            QJsonDocument errorDoc = QJsonDocument::fromJson(errorResponse);
            if (!errorDoc.isNull() && errorDoc.isObject()) {
                responseData = errorDoc.object();

                // Extract error message if available
                if (responseData.contains("message")) {
                    QString errorMessage = responseData["message"].toString();
                    LOG_ERROR(QString("Server error message: %1").arg(errorMessage));
                    m_lastErrorMessage = errorMessage;
                }

                // Extract error code if available in response
                if (responseData.contains("code")) {
                    QString code = responseData["code"].toString();
                    LOG_ERROR(QString("Server error code: %1").arg(code));
                }
            }
        }

        // Special handling for specific error codes
        switch (m_lastErrorCode) {
            case 400:  // Bad Request
                LOG_ERROR("Bad request format - check request parameters");
                break;
            case 401:  // Unauthorized
                LOG_ERROR("Authentication required - token may be expired");
                m_authToken.clear(); // Clear the token to force reauthentication
                break;
            case 403:  // Forbidden
                LOG_ERROR("Access forbidden - insufficient permissions");
                break;
            case 404:  // Not Found
                LOG_ERROR(QString("Resource not found: %1").arg(requestUrl));
                break;
            case 500:  // Server Error
                LOG_ERROR("Server internal error");
                break;
            case 503:  // Service Unavailable
                LOG_ERROR("Service unavailable - server may be overloaded");
                break;
            default:
                if (m_lastErrorCode >= 400) {
                    LOG_ERROR(QString("HTTP error: %1").arg(m_lastErrorCode));
                } else if (error == QNetworkReply::TimeoutError) {
                    LOG_ERROR("Request timed out");
                } else if (error == QNetworkReply::ConnectionRefusedError) {
                    LOG_ERROR("Connection refused");
                } else if (error == QNetworkReply::HostNotFoundError) {
                    LOG_ERROR("Host not found");
                }
                break;
        }

        return false;
    }

    // Read the response data
    QByteArray responseBytes = reply->readAll();

    // Log response at debug level (truncated for large responses)
    if (responseBytes.size() <= 1024) {
        LOG_DEBUG(QString("Response: %1").arg(QString::fromUtf8(responseBytes)));
    } else {
        LOG_DEBUG(QString("Response (truncated): %1...").arg(QString::fromUtf8(responseBytes.left(1024))));
    }

    // Check for empty response
    if (responseBytes.isEmpty()) {
        // Some endpoints return empty responses for success
        // If we got here with no network error, consider it a success
        responseData = QJsonObject(); // Empty object
        m_lastErrorCode = 0;
        m_lastErrorMessage.clear();
        return true;
    }

    // Parse JSON response
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseBytes, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1 at offset %2")
                 .arg(parseError.errorString())
                 .arg(parseError.offset));
        m_lastErrorMessage = "JSON parse error: " + parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        LOG_ERROR("Response is not a JSON object");
        m_lastErrorMessage = "Response is not a JSON object";
        return false;
    }

    responseData = doc.object();

    // Check for error response from server
    if (responseData.contains("error") && responseData["error"].toBool()) {
        QString errorMessage = "Server error";
        if (responseData.contains("message")) {
            errorMessage = responseData["message"].toString();
        }
        LOG_ERROR("Server error: " + errorMessage);
        m_lastErrorMessage = errorMessage;
        return false;
    }

    // Success
    m_lastErrorCode = 0;
    m_lastErrorMessage.clear();
    return true;
}

bool APIManager::logout(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_INFO("Logging out user");

    // Empty post data
    QJsonObject emptyData;

    return sendRequest("auth/logout", emptyData, responseData);
}

bool APIManager::getUserProfile(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_DEBUG("Getting user profile");

    return sendRequest("auth/profile", QJsonObject(), responseData, "GET");
}

bool APIManager::refreshToken(const QString &refreshToken, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_DEBUG("Refreshing authentication token");

    QJsonObject tokenData;
    tokenData["refresh_token"] = refreshToken;

    bool success = sendRequest("auth/refresh", tokenData, responseData);

    if (success && responseData.contains("token")) {
        QMutexLocker locker(&m_mutex);
        m_authToken = responseData["token"].toString();
        LOG_INFO("Token refreshed successfully");
        return true;
    }

    LOG_ERROR("Failed to refresh token");
    return false;
}

bool APIManager::getAllSessions(bool activeOnly, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions";
    if (activeOnly) {
        endpoint += "?active=true";
    }

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getActiveSession(const QString &machineId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/active";
    if (!machineId.isEmpty()) {
        endpoint += "?machine_id=" + machineId;
    }

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getSessionsByUser(const QString &userId, bool activeOnly, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "users/" + userId + "/sessions";
    if (activeOnly) {
        endpoint += "?active=true";
    }

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getSessionsByMachine(const QString &machineId, bool activeOnly, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "machines/" + machineId + "/sessions";
    if (activeOnly) {
        endpoint += "?active=true";
    }

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getSessionStats(const QUuid &sessionId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/stats";

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getUserStats(const QString &userId, const QDate &startDate, const QDate &endDate, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QUrlQuery query;
    if (startDate.isValid()) {
        query.addQueryItem("start_date", startDate.toString());
    }
    if (endDate.isValid()) {
        query.addQueryItem("end_date", endDate.toString());
    }

    QString endpoint = "users/" + userId + "/stats";
    if (!query.isEmpty()) {
        endpoint += "?" + query.toString();
    }

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getSessionChain(const QUuid &sessionId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/chain";

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getAfkPeriods(const QUuid &sessionId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/afk";

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getSessionEvents(const QUuid &sessionId, int limit, int offset, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QUrlQuery query;
    if (limit > 0) {
        query.addQueryItem("limit", QString::number(limit));
    }
    if (offset > 0) {
        query.addQueryItem("offset", QString::number(offset));
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/events";
    if (!query.isEmpty()) {
        endpoint += "?" + query.toString();
    }

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::createSessionEvent(const QUuid &sessionId, const QJsonObject &eventData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/events";

    return sendRequest(endpoint, eventData, responseData);
}

bool APIManager::createActivityEvent(const QUuid &sessionId, const QJsonObject &eventData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/activities";

    return sendRequest(endpoint, eventData, responseData);
}

bool APIManager::getAppUsages(const QUuid &sessionId, bool activeOnly, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/app-usages";
    if (activeOnly) {
        endpoint += "?active=true";
    }

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getAppUsageStats(const QUuid &sessionId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/app-usages/stats";

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getTopApps(const QUuid &sessionId, int limit, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/app-usages/top";
    if (limit > 0) {
        endpoint += "?limit=" + QString::number(limit);
    }

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getActiveApps(const QUuid &sessionId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/app-usages/active";

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getAllApplications(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("applications", QJsonObject(), responseData, "GET");
}

bool APIManager::getApplication(const QString &appId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "applications/" + appId;

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::createApplication(const QJsonObject &appData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("applications", appData, responseData);
}

bool APIManager::updateApplication(const QString &appId, const QJsonObject &appData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "applications/" + appId;

    return sendRequest(endpoint, appData, responseData, "PUT");
}

bool APIManager::deleteApplication(const QString &appId)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "applications/" + appId;
    QJsonObject responseData;

    return sendRequest(endpoint, QJsonObject(), responseData, "DELETE");
}

bool APIManager::getRestrictedApplications(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("applications/restricted", QJsonObject(), responseData, "GET");
}

bool APIManager::getTrackedApplications(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("applications/tracked", QJsonObject(), responseData, "GET");
}

bool APIManager::detectApplication(const QJsonObject &appData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_DEBUG("Detecting application");
    return sendRequest("applications/detect", appData, responseData);
}

bool APIManager::getSystemMetrics(const QUuid &sessionId, int limit, int offset, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QUrlQuery query;
    if (limit > 0) {
        query.addQueryItem("limit", QString::number(limit));
    }
    if (offset > 0) {
        query.addQueryItem("offset", QString::number(offset));
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/metrics";
    if (!query.isEmpty()) {
        endpoint += "?" + query.toString();
    }

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::recordSystemMetrics(const QUuid &sessionId, const QJsonObject &metricsData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/metrics";

    return sendRequest(endpoint, metricsData, responseData);
}

bool APIManager::getAverageMetrics(const QUuid &sessionId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/metrics/average";

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getMetricsTimeSeries(const QUuid &sessionId, const QString &metricType, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString() + "/metrics/timeseries/" + metricType;

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getCurrentSystemInfo(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("system/info", QJsonObject(), responseData, "GET");
}

bool APIManager::getAllMachines(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("machines", QJsonObject(), responseData, "GET");
}

bool APIManager::getActiveMachines(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("machines/active", QJsonObject(), responseData, "GET");
}

bool APIManager::getMachineByName(const QString &name, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "machine/name/" + name;

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::getMachine(const QString &machineId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "machines/" + machineId;

    return sendRequest(endpoint, QJsonObject(), responseData, "GET");
}

bool APIManager::createMachine(const QJsonObject &machineData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("machines", machineData, responseData);
}

bool APIManager::registerMachine(const QJsonObject &machineData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("machines/register", machineData, responseData, "POST", false);
}

bool APIManager::updateMachine(const QString &machineId, const QJsonObject &machineData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "machines/" + machineId;

    return sendRequest(endpoint, machineData, responseData, "PUT");
}

bool APIManager::updateMachineStatus(const QString &machineId, bool active, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "machines/" + machineId + "/status";
    QJsonObject statusData;
    statusData["active"] = active;

    return sendRequest(endpoint, statusData, responseData, "PUT");
}

bool APIManager::updateMachineLastSeen(const QString &machineId, const QDateTime &timestamp, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "machines/" + machineId + "/lastseen";
    QJsonObject timeData;
    if (timestamp.isValid()) {
        timeData["timestamp"] = timestamp.toUTC().toString();
    }

    return sendRequest(endpoint, timeData, responseData, "PUT");
}

bool APIManager::deleteMachine(const QString &machineId, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "machines/" + machineId;

    return sendRequest(endpoint, QJsonObject(), responseData, "DELETE");
}

bool APIManager::processBatch(const QJsonObject &batchData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("batch", batchData, responseData);
}

bool APIManager::processSessionBatch(const QUuid &sessionId, const QJsonObject &batchData, QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    QString endpoint = "sessions/" + sessionId.toString().remove('{').remove('}') + "/batch";

    return sendRequest(endpoint, batchData, responseData);
}

bool APIManager::ping(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("status/ping", QJsonObject(), responseData, "GET", false);
}

bool APIManager::getServerHealth(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("status/health", QJsonObject(), responseData, "GET");
}

bool APIManager::getServerVersion(QJsonObject &responseData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    return sendRequest("status/version", QJsonObject(), responseData, "GET", false);
}

bool APIManager::getServerConfiguration(QJsonObject& configData)
{
    if (!m_initialized) {
        LOG_ERROR("APIManager not initialized");
        return false;
    }

    LOG_DEBUG("Fetching server configuration");

    return sendRequest("config", QJsonObject(), configData, "GET");
}

bool APIManager::isAuthenticated() const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return !m_authToken.isEmpty();
}

QString APIManager::getAuthToken()
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));

    // If we don't have a token, return empty string
    // The caller should handle authentication in this case
    return m_authToken;
}
bool APIManager::setAuthToken(const QString& token)
{
    if (token.isEmpty()) {
        LOG_WARNING("Attempted to set empty authentication token");
        return false;
    }

    QMutexLocker locker(&m_mutex);
    m_authToken = token;
    LOG_INFO("Authentication token set directly");
    return true;
}

