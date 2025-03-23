#ifndef HTTP_CONTROLLER_H
#define HTTP_CONTROLLER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QUuid>
#include <QDateTime>
#include <QMap>
#include "logger/logger.h"

namespace Http {

    class Controller : public QObject {
        Q_OBJECT
    public:
        explicit Controller(QObject* parent = nullptr);
        virtual ~Controller();

        virtual void setupRoutes(QHttpServer& server) = 0;

        // New method to check initialization status
        bool isInitialized() const { return m_initialized; }

        // New method to get controller name for logging
        virtual QString getControllerName() const = 0;

    protected:
        // Request parsing helpers
        QJsonObject extractJsonFromRequest(const QHttpServerRequest& request, bool& ok) const;
        QMap<QString, QString> getQueryParams(const QHttpServerRequest& request) const;
        QMap<QString, QVariant> getQueryParamsWithTypes(const QHttpServerRequest& request) const;

        // Parameter parsing helpers
        int getIntParam(const QMap<QString, QString>& params, const QString& name, int defaultValue = 0) const;
        bool getBoolParam(const QMap<QString, QString>& params, const QString& name, bool defaultValue = false) const;
        QDateTime getDateTimeParam(const QMap<QString, QString>& params, const QString& name,
                                  const QDateTime& defaultValue = QDateTime()) const;

        // UUID helpers
        QUuid stringToUuid(const QString& str) const;
        QString uuidToString(const QUuid& uuid) const;

        // Time helpers
        qint64 calculateDuration(const QDateTime& startTime, const QDateTime& endTime = QDateTime()) const;

        // Validation helpers
        bool validateRequiredFields(const QJsonObject& data, const QStringList& fields, QStringList& missingFields) const;
        bool validateFieldTypes(const QJsonObject& data, const QMap<QString, QString>& fieldTypes,
                              QStringList& typeErrors) const;

        // Response helpers
        QHttpServerResponse createResponse(const QJsonObject& data,
                                         QHttpServerResponder::StatusCode status = QHttpServerResponder::StatusCode::Ok);
        QHttpServerResponse createResponse(const QJsonArray& data,
                                         QHttpServerResponder::StatusCode status = QHttpServerResponder::StatusCode::Ok);
        QHttpServerResponse createErrorResponse(const QString& message, QHttpServerResponder::StatusCode status);
        QHttpServerResponse createValidationErrorResponse(const QStringList& errors);

        // Logging helpers
        void logRequestReceived(const QHttpServerRequest& request) const;
        void logRequestCompleted(const QHttpServerRequest& request, QHttpServerResponder::StatusCode status) const;

        // Track initialization status
        bool m_initialized = false;

        // Route builders (to be implemented by derived classes)
        virtual void registerGetRoutes(QHttpServer& server) {}
        virtual void registerPostRoutes(QHttpServer& server) {}
        virtual void registerPutRoutes(QHttpServer& server) {}
        virtual void registerDeleteRoutes(QHttpServer& server) {}
    };

} // namespace Http

#endif // HTTP_CONTROLLER_H