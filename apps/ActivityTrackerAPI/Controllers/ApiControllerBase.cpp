// ApiControllerBase.cpp
#include "ApiControllerBase.h"
#include "logger/logger.h"
#include "httpserver/response.h"

ApiControllerBase::ApiControllerBase(QObject* parent)
    : Http::Controller(parent)
{
    LOG_DEBUG("ApiControllerBase created");
}

ApiControllerBase::~ApiControllerBase()
{
    LOG_DEBUG("ApiControllerBase destroyed");
}

bool ApiControllerBase::isUserAuthorized(const QHttpServerRequest& request, QJsonObject& userData, bool strictMode)
{
    // Use AuthFramework directly
    bool authorized = AuthFramework::instance().authorizeRequest(request, userData, strictMode);

    if (authorized) {
        LOG_DEBUG(QString("Request authorized for user: %1").arg(
            userData.contains("name") ? userData["name"].toString() : "unknown"));
    } else {
        LOG_WARNING("Request not authorized");
    }

    return authorized;
}

bool ApiControllerBase::requiresRole(const QHttpServerRequest& request, const QString& role, QJsonObject& userData)
{
    // Delegate to AuthFramework
    return AuthFramework::instance().requiresRole(request, role, userData);
}

QString ApiControllerBase::getControllerName() const
{
    // Default implementation - derived classes should override
    return "ApiController";
}

QHttpServerResponse ApiControllerBase::createSuccessResponse(const QJsonObject& data, QHttpServerResponder::StatusCode status)
{
    return Http::Response::json(data, status);
}

QHttpServerResponse ApiControllerBase::createSuccessResponse(const QJsonArray& data, QHttpServerResponder::StatusCode status)
{
    // For QJsonArray, there's no overload with status, so we need to handle it differently
    if (status == QHttpServerResponder::StatusCode::Ok) {
        return Http::Response::json(data);
    } else {
        // Create a wrapper object if we need a different status
        QJsonObject wrapper;
        wrapper["data"] = data;
        return Http::Response::json(wrapper, status);
    }
}

QHttpServerResponse ApiControllerBase::createErrorResponse(const QString& message, QHttpServerResponder::StatusCode status)
{
    return Http::Response::error(message, status);
}

QHttpServerResponse ApiControllerBase::createValidationErrorResponse(const QStringList& errors)
{
    QJsonObject errorObj;
    errorObj["error"] = true;
    errorObj["message"] = "Validation failed";

    QJsonArray errorsArray;
    for (const QString& error : errors) {
        errorsArray.append(error);
    }

    errorObj["errors"] = errorsArray;

    return Http::Response::json(errorObj, QHttpServerResponder::StatusCode::BadRequest);
}

bool ApiControllerBase::isReportEndpoint(const QString& path) const
{
    return AuthFramework::instance().isReportEndpoint(path);
}

bool ApiControllerBase::isServiceTokenAuthorized(const QHttpServerRequest& request, QJsonObject& userData)
{
    // Extract service token
    QString serviceToken = AuthFramework::instance().extractServiceToken(request);
    if (!serviceToken.isEmpty()) {
        // Validate service token
        bool valid = AuthFramework::instance().validateServiceToken(serviceToken, userData);
        if (valid) {
            LOG_INFO(QString("Service token authorization successful for: %1").arg(userData["username"].toString()));
            return true;
        }
    }

    // Fall back to regular authorization
    return isUserAuthorized(request, userData, false);
}

