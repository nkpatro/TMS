// Enhanced Response.cpp
#include "httpserver/Response.h"
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include "logger/logger.h"

namespace Http {

    QHttpServerResponse Response::json(const QJsonObject& data) {
        return QHttpServerResponse(data, QHttpServerResponder::StatusCode::Ok);
    }

    QHttpServerResponse Response::json(const QJsonArray& data) {
        return QHttpServerResponse(data, QHttpServerResponder::StatusCode::Ok);
    }

    QHttpServerResponse Response::json(const QJsonObject& data, QHttpServerResponder::StatusCode statusCode) {
        return QHttpServerResponse(data, statusCode);
    }

    QHttpServerResponse Response::paginated(const QJsonArray& data, int total, int page, int pageSize, const QString& nextPage) {
        QJsonObject response;
        response["data"] = data;
        QJsonObject meta;
        meta["total"] = total;
        meta["page"] = page;
        meta["page_size"] = pageSize;
        meta["total_pages"] = (total + pageSize - 1) / pageSize;

        if (!nextPage.isEmpty()) {
            meta["next_page"] = nextPage;
        }

        if (page > 1) {
            meta["prev_page"] = QString::number(page - 1);
        }

        response["meta"] = meta;
        return QHttpServerResponse(response, QHttpServerResponder::StatusCode::Ok);
    }

    QHttpServerResponse Response::created(const QJsonObject& data) {
        return json(data, QHttpServerResponder::StatusCode::Created);
    }

    QHttpServerResponse Response::noContent() {
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    }

    QHttpServerResponse Response::badRequest(const QString& message, const QString& errorCode) {
        LOG_WARNING(QString("Bad Request: %1").arg(message));
        return error(message, QHttpServerResponder::StatusCode::BadRequest, errorCode);
    }

    QHttpServerResponse Response::unauthorized(const QString& message, const QString& errorCode) {
        LOG_WARNING(QString("Unauthorized: %1").arg(message));
        return error(message, QHttpServerResponder::StatusCode::Unauthorized, errorCode);
    }

    QHttpServerResponse Response::forbidden(const QString& message, const QString& errorCode) {
        LOG_WARNING(QString("Forbidden: %1").arg(message));
        return error(message, QHttpServerResponder::StatusCode::Forbidden, errorCode);
    }

    QHttpServerResponse Response::notFound(const QString& message, const QString& errorCode) {
        LOG_WARNING(QString("Not Found: %1").arg(message));
        return error(message, QHttpServerResponder::StatusCode::NotFound, errorCode);
    }

    QHttpServerResponse Response::methodNotAllowed(const QString& message, const QString& errorCode) {
        LOG_WARNING(QString("Method Not Allowed: %1").arg(message));
        return error(message, QHttpServerResponder::StatusCode::MethodNotAllowed, errorCode);
    }

    QHttpServerResponse Response::conflict(const QString& message, const QString& errorCode) {
        LOG_WARNING(QString("Conflict: %1").arg(message));
        return error(message, QHttpServerResponder::StatusCode::Conflict, errorCode);
    }

    QHttpServerResponse Response::unprocessableEntity(const QString& message, const QString& errorCode) {
        LOG_WARNING(QString("Unprocessable Entity: %1").arg(message));
        return error(message, QHttpServerResponder::StatusCode::UnprocessableEntity, errorCode);
    }

    QHttpServerResponse Response::internalError(const QString& message, const QString& errorCode) {
        LOG_ERROR(QString("Internal Error: %1").arg(message));
        return error(message, QHttpServerResponder::StatusCode::InternalServerError, errorCode);
    }

    QHttpServerResponse Response::serviceUnavailable(const QString& message, const QString& errorCode) {
        LOG_ERROR(QString("Service Unavailable: %1").arg(message));
        return error(message, QHttpServerResponder::StatusCode::ServiceUnavailable, errorCode);
    }

    QHttpServerResponse Response::error(const QString& message, QHttpServerResponder::StatusCode statusCode, const QString& errorCode) {
        QJsonObject errorResponse{
            {"error", true},
            {"message", message}
        };

        if (!errorCode.isEmpty()) {
            errorResponse["code"] = errorCode;
        }

        return QHttpServerResponse(errorResponse, statusCode);
    }

    QHttpServerResponse Response::validationError(const QString& message, const QMap<QString, QString>& fieldErrors) {
        LOG_WARNING(QString("Validation Error: %1").arg(message));

        QJsonObject errorResponse{
            {"error", true},
            {"message", message},
            {"code", "VALIDATION_ERROR"}
        };

        if (!fieldErrors.isEmpty()) {
            QJsonObject fields;
            for (auto it = fieldErrors.constBegin(); it != fieldErrors.constEnd(); ++it) {
                fields[it.key()] = it.value();
            }
            errorResponse["fields"] = fields;
        }

        return QHttpServerResponse(errorResponse, QHttpServerResponder::StatusCode::UnprocessableEntity);
    }

    QHttpServerResponse Response::file(const QString& path, const QString& mimeType) {
        QFile file(path);
        if (!file.exists()) {
            return notFound("File not found");
        }

        if (!file.open(QIODevice::ReadOnly)) {
            return internalError("Unable to read file");
        }

        QString actualMimeType = mimeType;
        if (actualMimeType.isEmpty()) {
            QMimeDatabase db;
            actualMimeType = db.mimeTypeForFile(path).name();
        }

        // Create a response with the file content
        QByteArray fileData = file.readAll();
        QByteArray contentType = actualMimeType.toUtf8();

        return QHttpServerResponse(fileData, contentType, QHttpServerResponder::StatusCode::Ok);
    }

    QHttpServerResponse Response::download(const QString& path, const QString& filename) {
        // Note: In Qt 6.8, we can't directly set the Content-Disposition header
        // This method now works like the file() method but logs a warning that
        // the Content-Disposition header can't be set

        QFile file(path);
        if (!file.exists()) {
            return notFound("File not found");
        }

        if (!file.open(QIODevice::ReadOnly)) {
            return internalError("Unable to read file");
        }

        // Determine content type
        QMimeDatabase db;
        QString mimeType = db.mimeTypeForFile(path).name();

        // Get file data
        QByteArray fileData = file.readAll();

        // Log a warning that we can't set the Content-Disposition header
        LOG_WARNING("Content-Disposition header not supported in this Qt version. File will be served without download prompt.");

        // Return the file with the correct content type
        return QHttpServerResponse(fileData, mimeType.toUtf8(), QHttpServerResponder::StatusCode::Ok);
    }

    QHttpServerResponse Response::stream(const QByteArray& data, const QString& mimeType) {
        return QHttpServerResponse(data, mimeType.toUtf8(), QHttpServerResponder::StatusCode::Ok);
    }
} // namespace Http