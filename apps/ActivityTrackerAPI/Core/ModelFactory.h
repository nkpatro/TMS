#ifndef MODELFACTORY_H
#define MODELFACTORY_H

#include <QSqlQuery>
#include <QDateTime>
#include <QMap>
#include <QVariant>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QHostAddress>
#include <QSharedPointer>

// Forward declarations for all model types
class UserModel;
class MachineModel;
class SessionModel;
class ActivityEventModel;
class AfkPeriodModel;
class ApplicationModel;
class AppUsageModel;
class DisciplineModel;
class SystemMetricsModel;
class RoleModel;
class SessionEventModel;
class UserRoleDisciplineModel;

/**
 * @brief Factory class for creating model instances
 *
 * This class centralizes model creation logic to ensure consistent initialization
 * across the application. It provides methods for creating models from database
 * results and for initializing default model instances.
 */
class ModelFactory {
public:
    // Create models from query results
    static UserModel* createUserFromQuery(const QSqlQuery& query);
    static MachineModel* createMachineFromQuery(const QSqlQuery& query);
    static SessionModel* createSessionFromQuery(const QSqlQuery& query);
    static ActivityEventModel* createActivityEventFromQuery(const QSqlQuery& query);
    static AfkPeriodModel* createAfkPeriodFromQuery(const QSqlQuery& query);
    static ApplicationModel* createApplicationFromQuery(const QSqlQuery& query);
    static AppUsageModel* createAppUsageFromQuery(const QSqlQuery& query);
    static DisciplineModel* createDisciplineFromQuery(const QSqlQuery& query);
    static SystemMetricsModel* createSystemMetricsFromQuery(const QSqlQuery& query);
    static RoleModel* createRoleFromQuery(const QSqlQuery& query);
    static SessionEventModel* createSessionEventFromQuery(const QSqlQuery& query);
    static UserRoleDisciplineModel* createUserRoleDisciplineFromQuery(const QSqlQuery& query);

    // Create default models
    static UserModel* createDefaultUser(const QString& name = QString(), const QString& email = QString());
    static MachineModel* createDefaultMachine(const QString& name = QString());
    static SessionModel* createDefaultSession(const QUuid& userId = QUuid(), const QUuid& machineId = QUuid());
    static ActivityEventModel* createDefaultActivityEvent(const QUuid& sessionId = QUuid());
    static AfkPeriodModel* createDefaultAfkPeriod(const QUuid& sessionId = QUuid());
    static ApplicationModel* createDefaultApplication(const QString& name = QString(), const QString& appPath = QString());
    static AppUsageModel* createDefaultAppUsage(const QUuid& sessionId = QUuid(), const QUuid& appId = QUuid());
    static DisciplineModel* createDefaultDiscipline(const QString& name = QString());
    static SystemMetricsModel* createDefaultSystemMetrics(const QUuid& sessionId = QUuid());
    static RoleModel* createDefaultRole(const QString& name = QString(), const QString& code = QString());
    static SessionEventModel* createDefaultSessionEvent(const QUuid& sessionId = QUuid());
    static UserRoleDisciplineModel* createDefaultUserRoleDiscipline(const QUuid& userId = QUuid(), const QUuid& roleId = QUuid());

    // Model validation functions
    static bool validateUserModel(const UserModel* model, QStringList& errors);
    static bool validateMachineModel(const MachineModel* model, QStringList& errors);
    static bool validateSessionModel(const SessionModel* model, QStringList& errors);
    static bool validateActivityEventModel(const ActivityEventModel* model, QStringList& errors);
    static bool validateAfkPeriodModel(const AfkPeriodModel* model, QStringList& errors);
    static bool validateApplicationModel(const ApplicationModel* model, QStringList& errors);
    static bool validateAppUsageModel(const AppUsageModel* model, QStringList& errors);
    static bool validateDisciplineModel(const DisciplineModel* model, QStringList& errors);
    static bool validateSystemMetricsModel(const SystemMetricsModel* model, QStringList& errors);
    static bool validateRoleModel(const RoleModel* model, QStringList& errors);
    static bool validateSessionEventModel(const SessionEventModel* model, QStringList& errors);
    static bool validateUserRoleDisciplineModel(const UserRoleDisciplineModel* model, QStringList& errors);

    // Timestamp management
    static void setCreationTimestamps(QObject* model, const QUuid& userId = QUuid());
    static void setUpdateTimestamps(QObject* model, const QUuid& userId = QUuid());

    // JSON conversion utilities
    static QJsonObject modelToJson(const UserModel* model);
    static QJsonObject modelToJson(const MachineModel* model);
    static QJsonObject modelToJson(const SessionModel* model);
    static QJsonObject modelToJson(const ActivityEventModel* model);
    static QJsonObject modelToJson(const AfkPeriodModel* model);
    static QJsonObject modelToJson(const ApplicationModel* model);
    static QJsonObject modelToJson(const AppUsageModel* model);
    static QJsonObject modelToJson(const DisciplineModel* model);
    static QJsonObject modelToJson(const SystemMetricsModel* model);
    static QJsonObject modelToJson(const RoleModel* model);
    static QJsonObject modelToJson(const SessionEventModel* model);
    static QJsonObject modelToJson(const UserRoleDisciplineModel* model);

    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<UserModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<MachineModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<SessionModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<ActivityEventModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<AfkPeriodModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<ApplicationModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<AppUsageModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<DisciplineModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<SystemMetricsModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<RoleModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<SessionEventModel>>& models);
    static QJsonArray modelsToJsonArray(const QList<QSharedPointer<UserRoleDisciplineModel>>& models);

private:
    // Helper method to set common base model fields
    template<typename T>
    static void setBaseModelFields(T* model, const QSqlQuery& query);

    // Helper methods for validation
    static bool validateRequiredFields(const QMap<QString, QVariant>& fields, QStringList& errors);

    // Helpers for query value extraction with default values
    static QUuid getUuidOrDefault(const QSqlQuery& query, const QString& fieldName, const QUuid& defaultValue = QUuid());
    static QString getStringOrDefault(const QSqlQuery& query, const QString& fieldName, const QString& defaultValue = QString());
    static int getIntOrDefault(const QSqlQuery& query, const QString& fieldName, int defaultValue = 0);
    static double getDoubleOrDefault(const QSqlQuery& query, const QString& fieldName, double defaultValue = 0.0);
    static bool getBoolOrDefault(const QSqlQuery& query, const QString& fieldName, bool defaultValue = false);
    static QDateTime getDateTimeOrDefault(const QSqlQuery& query, const QString& fieldName, const QDateTime& defaultValue = QDateTime());
    static QJsonObject getJsonObjectOrDefault(const QSqlQuery& query, const QString& fieldName, const QJsonObject& defaultValue = QJsonObject());
    static QJsonArray getJsonArrayOrDefault(const QSqlQuery& query, const QString& fieldName, const QJsonArray& defaultValue = QJsonArray());
    static QHostAddress getHostAddressOrDefault(const QSqlQuery& query, const QString& fieldName, const QHostAddress& defaultValue = QHostAddress());
};

#endif // MODELFACTORY_H