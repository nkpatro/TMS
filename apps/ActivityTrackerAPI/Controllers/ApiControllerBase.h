#ifndef APICONTROLLERBASE_H
#define APICONTROLLERBASE_H

#include "httpserver/controller.h"
#include "Core/AuthFramework.h"

class ApiControllerBase : public Http::Controller
{
    Q_OBJECT
public:
    explicit ApiControllerBase(QObject* parent = nullptr);
    virtual ~ApiControllerBase();

    // Auth methods that use AuthFramework
    bool isUserAuthorized(const QHttpServerRequest& request, QJsonObject& userData, bool strictMode = false);
    bool requiresRole(const QHttpServerRequest& request, const QString& role, QJsonObject& userData);

    // Get controller name for logging
    QString getControllerName() const override;

protected:
    // Helper methods for controllers
    QHttpServerResponse createSuccessResponse(const QJsonObject& data, QHttpServerResponder::StatusCode status = QHttpServerResponder::StatusCode::Ok);
    QHttpServerResponse createSuccessResponse(const QJsonArray& data, QHttpServerResponder::StatusCode status = QHttpServerResponder::StatusCode::Ok);
    QHttpServerResponse createErrorResponse(const QString& message, QHttpServerResponder::StatusCode status = QHttpServerResponder::StatusCode::BadRequest);
    QHttpServerResponse createValidationErrorResponse(const QStringList& errors);

    // Convenience method for checking if a request is for a reporting endpoint
    bool isReportEndpoint(const QString& path) const;

    // For authorization based on service tokens (Activity Tracker specific)
    bool isServiceTokenAuthorized(const QHttpServerRequest& request, QJsonObject& userData);
};

#endif // APICONTROLLERBASE_H