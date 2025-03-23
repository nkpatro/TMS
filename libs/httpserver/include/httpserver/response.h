#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <QHttpServerResponse>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QMimeDatabase>

namespace Http {

    class Response {
    public:
        // Success responses
        static QHttpServerResponse json(const QJsonObject& data);
        static QHttpServerResponse json(const QJsonArray& data);
        static QHttpServerResponse json(const QJsonObject& data, QHttpServerResponder::StatusCode statusCode);
        static QHttpServerResponse created(const QJsonObject& data);
        static QHttpServerResponse noContent();

        // Paginated responses
        static QHttpServerResponse paginated(
            const QJsonArray& data,
            int total,
            int page,
            int pageSize,
            const QString& nextPage = QString());

        // Error responses
        static QHttpServerResponse badRequest(const QString& message, const QString& errorCode = "BAD_REQUEST");
        static QHttpServerResponse unauthorized(const QString& message = "Unauthorized", const QString& errorCode = "UNAUTHORIZED");
        static QHttpServerResponse forbidden(const QString& message = "Forbidden", const QString& errorCode = "FORBIDDEN");
        static QHttpServerResponse notFound(const QString& message = "Resource not found", const QString& errorCode = "NOT_FOUND");
        static QHttpServerResponse methodNotAllowed(const QString& message = "Method not allowed", const QString& errorCode = "METHOD_NOT_ALLOWED");
        static QHttpServerResponse conflict(const QString& message = "Resource conflict", const QString& errorCode = "CONFLICT");
        static QHttpServerResponse unprocessableEntity(const QString& message = "Unprocessable entity", const QString& errorCode = "UNPROCESSABLE_ENTITY");
        static QHttpServerResponse internalError(const QString& message = "Internal server error", const QString& errorCode = "INTERNAL_ERROR");
        static QHttpServerResponse serviceUnavailable(const QString& message = "Service unavailable", const QString& errorCode = "SERVICE_UNAVAILABLE");
        static QHttpServerResponse error(const QString& message, QHttpServerResponder::StatusCode statusCode, const QString& errorCode = "ERROR");

        // File responses
        static QHttpServerResponse file(const QString& path, const QString& mimeType = QString());
        static QHttpServerResponse download(const QString& path, const QString& filename);

        // Streaming responses
        static QHttpServerResponse stream(const QByteArray& data, const QString& mimeType);

        // Validation error response with field errors
        static QHttpServerResponse validationError(
            const QString& message,
            const QMap<QString, QString>& fieldErrors);
    };

} // namespace Http

#endif // HTTP_RESPONSE_H