#include "httpserver/controller.h"
#include "logger/logger.h"
#include <QJsonDocument>
#include <QUrlQuery>
#include <QDateTime>

namespace Http {

    Controller::Controller(QObject* parent)
        : QObject(parent), m_initialized(false)
    {
        LOG_DEBUG(QString("%1 created").arg(getControllerName()));
    }

    Controller::~Controller() {
        LOG_DEBUG(QString("%1 destroyed").arg(getControllerName()));
    }

    QJsonObject Controller::extractJsonFromRequest(const QHttpServerRequest& request, bool& ok) const {
        ok = false;
        QByteArray body = request.body();

        if (body.isEmpty()) {
            LOG_DEBUG("Request body is empty");
            return QJsonObject();
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            LOG_WARNING(QString("JSON parse error: %1").arg(parseError.errorString()));
            return QJsonObject();
        }

        if (!doc.isObject()) {
            LOG_WARNING("Request body is not a JSON object");
            return QJsonObject();
        }

        ok = true;
        LOG_DEBUG(QString("Extracted JSON: %1").arg(QString::fromUtf8(body).left(500) + (body.size() > 500 ? "..." : "")));
        return doc.object();
    }

    QMap<QString, QString> Controller::getQueryParams(const QHttpServerRequest& request) const {
        QMap<QString, QString> params;
        QUrlQuery query(request.url().query());

        for (const auto& item : query.queryItems()) {
            params[item.first] = item.second;
        }

        return params;
    }

    QMap<QString, QVariant> Controller::getQueryParamsWithTypes(const QHttpServerRequest& request) const {
        QMap<QString, QVariant> params;
        QUrlQuery query(request.url().query());

        for (const auto& item : query.queryItems()) {
            QString key = item.first;
            QString value = item.second;

            // Try to determine the type of the value
            bool boolOk = false;
            bool intOk = false;
            bool doubleOk = false;

            // Check if it's a boolean
            if (value.toLower() == "true" || value.toLower() == "false") {
                params[key] = (value.toLower() == "true");
                continue;
            }

            // Check if it's an integer
            int intValue = value.toInt(&intOk);
            if (intOk) {
                params[key] = intValue;
                continue;
            }

            // Check if it's a double
            double doubleValue = value.toDouble(&doubleOk);
            if (doubleOk) {
                params[key] = doubleValue;
                continue;
            }

            // Default to string
            params[key] = value;
        }

        return params;
    }

    int Controller::getIntParam(const QMap<QString, QString>& params, const QString& name, int defaultValue) const {
        if (!params.contains(name)) {
            return defaultValue;
        }

        bool ok;
        int value = params[name].toInt(&ok);

        return ok ? value : defaultValue;
    }

    bool Controller::getBoolParam(const QMap<QString, QString>& params, const QString& name, bool defaultValue) const {
        if (!params.contains(name)) {
            return defaultValue;
        }

        QString value = params[name].toLower();

        if (value == "true" || value == "1" || value == "yes") {
            return true;
        } else if (value == "false" || value == "0" || value == "no") {
            return false;
        }

        return defaultValue;
    }

    QDateTime Controller::getDateTimeParam(const QMap<QString, QString>& params, const QString& name, const QDateTime& defaultValue) const {
        if (!params.contains(name)) {
            return defaultValue;
        }

        QDateTime value = QDateTime::fromString(params[name], Qt::ISODate);

        return value.isValid() ? value : defaultValue;
    }

    QUuid Controller::stringToUuid(const QString& str) const {
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

    QString Controller::uuidToString(const QUuid& uuid) const {
        return uuid.toString(QUuid::WithoutBraces);
    }

    qint64 Controller::calculateDuration(const QDateTime& startTime, const QDateTime& endTime) const {
        if (!startTime.isValid()) {
            return 0;
        }

        QDateTime end = endTime.isValid() ? endTime : QDateTime::currentDateTimeUtc();
        return startTime.secsTo(end);
    }

    bool Controller::validateRequiredFields(const QJsonObject& data, const QStringList& fields, QStringList& missingFields) const {
        missingFields.clear();

        for (const auto& field : fields) {
            if (!data.contains(field) || data[field].isNull() ||
                (data[field].isString() && data[field].toString().isEmpty())) {
                missingFields.append(field);
            }
        }

        return missingFields.isEmpty();
    }

    bool Controller::validateFieldTypes(const QJsonObject& data, const QMap<QString, QString>& fieldTypes, QStringList& typeErrors) const {
        typeErrors.clear();

        for (auto it = fieldTypes.constBegin(); it != fieldTypes.constEnd(); ++it) {
            const QString& fieldName = it.key();
            const QString& expectedType = it.value();

            // Skip if field is not in data
            if (!data.contains(fieldName)) {
                continue;
            }

            const QJsonValue& value = data[fieldName];

            // Check type
            bool typeOk = false;

            if (expectedType == "string") {
                typeOk = value.isString();
            } else if (expectedType == "number" || expectedType == "int" || expectedType == "double") {
                typeOk = value.isDouble();
            } else if (expectedType == "boolean" || expectedType == "bool") {
                typeOk = value.isBool();
            } else if (expectedType == "array") {
                typeOk = value.isArray();
            } else if (expectedType == "object") {
                typeOk = value.isObject();
            } else if (expectedType == "uuid") {
                typeOk = value.isString() && !QUuid(value.toString()).isNull();
            } else if (expectedType == "datetime") {
                typeOk = value.isString() && QDateTime::fromString(value.toString(), Qt::ISODate).isValid();
            }

            if (!typeOk) {
                typeErrors.append(QString("%1 must be a valid %2").arg(fieldName, expectedType));
            }
        }

        return typeErrors.isEmpty();
    }

    QHttpServerResponse Controller::createResponse(const QJsonObject& data, QHttpServerResponder::StatusCode status) {
        return QHttpServerResponse(data, status);
    }

    QHttpServerResponse Controller::createResponse(const QJsonArray& data, QHttpServerResponder::StatusCode status) {
        return QHttpServerResponse(data, status);
    }

    QHttpServerResponse Controller::createErrorResponse(const QString& message, QHttpServerResponder::StatusCode status) {
        QJsonObject error{
            {"error", true},
            {"message", message}
        };
        return QHttpServerResponse(error, status);
    }

    QHttpServerResponse Controller::createValidationErrorResponse(const QStringList& errors) {
        QJsonObject error{
            {"error", true},
            {"message", "Validation failed"},
            {"validationErrors", QJsonArray::fromStringList(errors)}
        };
        return QHttpServerResponse(error, QHttpServerResponder::StatusCode::BadRequest);
    }

    void Controller::logRequestReceived(const QHttpServerRequest& request) const {
        LOG_DEBUG(QString("[%1] Request received: %2 %3")
                 .arg(getControllerName(),
                      QString::number(static_cast<int>(request.method())),
                      request.url().toString()));
    }

    void Controller::logRequestCompleted(const QHttpServerRequest& request, QHttpServerResponder::StatusCode status) const {
        LOG_DEBUG(QString("[%1] Request completed: %2 %3 - Status: %4")
                 .arg(getControllerName(),
                      QString::number(static_cast<int>(request.method())),
                      request.url().toString(),
                      QString::number(static_cast<int>(status))));
    }

    QString Controller::getControllerName() const {
        // Default implementation - derived classes should override
        return QString("Controller");
    }

} // namespace Http