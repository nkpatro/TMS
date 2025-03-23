#ifndef AUTHHELPER_H
#define AUTHHELPER_H

#include <QJsonObject>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include "logger/logger.h"
//#include "Core/Response.h"
#include "Core/AuthFramework.h"

/**
 * @brief Helper class for authorization-related functionality
 *
 * This class provides static methods for authorization and authentication checks
 */
class AuthHelper {
public:
    /**
     * @brief Authorize a request using AuthFramework
     * @param request The HTTP request
     * @param userData Object to store user data if authorized
     * @param strictMode Whether to require valid authentication
     * @return True if authorized, false otherwise
     */
    static bool authorizeRequest(
        const QHttpServerRequest &request,
        QJsonObject &userData,
        bool strictMode = false
    ) {
        return AuthFramework::instance().authorizeRequest(request, userData, strictMode);
    }

    /**
     * @brief Authorize a request and return an HTTP response if not authorized
     * @param request The HTTP request
     * @param userData Object to store user data if authorized
     * @param strictMode Whether to require valid authentication
     * @return Empty response if authorized, error response if not
     */
    static QHttpServerResponse authorizeOrError(
        const QHttpServerRequest &request,
        QJsonObject &userData,
        bool strictMode = false
    ) {
        if (!authorizeRequest(request, userData, strictMode)) {
            LOG_WARNING("Unauthorized request");
            return Http::Response::unauthorized("Unauthorized");
        }
        return QHttpServerResponse(); // Empty response indicates success
    }

    /**
     * @brief Check if a path is for a reporting endpoint
     * @param path The endpoint path
     * @return True if it's a reporting endpoint, false otherwise
     */
    static bool isReportEndpoint(const QString &path) {
        return AuthFramework::instance().isReportEndpoint(path);
    }
};

#endif // AUTHHELPER_H