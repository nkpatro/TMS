#include "Repositories/ApplicationRepository.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include "Core/ModelFactory.h"

ApplicationRepository::ApplicationRepository(QObject *parent)
    : BaseRepository<ApplicationModel>(parent)
{
    LOG_DEBUG("ApplicationRepository created");
}

QString ApplicationRepository::getEntityName() const
{
    return "Application";
}

QString ApplicationRepository::getModelId(ApplicationModel* model) const
{
    return model->id().toString();
}

QString ApplicationRepository::buildSaveQuery()
{
    return "INSERT INTO applications "
           "(app_name, app_path, app_hash, is_restricted, tracking_enabled, "
           "created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:app_name, :app_path, :app_hash, :is_restricted, :tracking_enabled, "
           ":created_at, :created_by, :updated_at, :updated_by) "
           "RETURNING id";
}

QString ApplicationRepository::buildUpdateQuery()
{
    return "UPDATE applications SET "
           "app_name = :app_name, "
           "app_path = :app_path, "
           "app_hash = :app_hash, "
           "is_restricted = :is_restricted, "
           "tracking_enabled = :tracking_enabled, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE id = :id";
}

QString ApplicationRepository::buildGetByIdQuery()
{
    return "SELECT * FROM applications WHERE id = :id";
}

QString ApplicationRepository::buildGetAllQuery()
{
    return "SELECT * FROM applications ORDER BY app_name";
}

QString ApplicationRepository::buildRemoveQuery()
{
    return "DELETE FROM applications WHERE id = :id";
}

QMap<QString, QVariant> ApplicationRepository::prepareParamsForSave(ApplicationModel *application)
{
    QMap<QString, QVariant> params;
    params["app_name"] = application->appName();
    params["app_path"] = application->appPath();
    params["app_hash"] = application->appHash();
    params["is_restricted"] = application->isRestricted() ? "true" : "false";
    params["tracking_enabled"] = application->trackingEnabled() ? "true" : "false";
    params["created_at"] = application->createdAt().toUTC();
    params["created_by"] = application->createdBy().isNull() ? QVariant(QVariant::Invalid) : application->createdBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = application->updatedAt().toUTC();
    params["updated_by"] = application->updatedBy().isNull() ? QVariant(QVariant::Invalid) : application->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

QMap<QString, QVariant> ApplicationRepository::prepareParamsForUpdate(ApplicationModel *application)
{
    QMap<QString, QVariant> params = prepareParamsForSave(application);
    params["id"] = application->id().toString(QUuid::WithoutBraces);
    return params;
}

ApplicationModel* ApplicationRepository::createModelFromQuery(const QSqlQuery &query)
{
    // Use ModelFactory to create the model
    return ModelFactory::createApplicationFromQuery(query);
}

QSharedPointer<ApplicationModel> ApplicationRepository::getByPath(const QString &appPath)
{
    LOG_DEBUG(QString("Get Application by Path: %1").arg(appPath));

    if (!ensureInitialized() || appPath.isEmpty()) {
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["app_path"] = appPath;
    QString query = "SELECT * FROM applications WHERE app_path = :app_path";

    logQueryWithValues(query, params);
    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> ApplicationModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("Application found by Path: %1").arg(appPath));
        return QSharedPointer<ApplicationModel>(*result);
    } else {
        LOG_WARNING(QString("Application not found with Path: %1").arg(appPath));
        return nullptr;
    }
}

QSharedPointer<ApplicationModel> ApplicationRepository::getByPathAndName(const QString &appPath, const QString &appName)
{
    LOG_DEBUG(QString("Get Application by Path and Name: %1").arg(appPath));

    if (!ensureInitialized() || appPath.isEmpty() || appName.isEmpty()) {
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["app_path"] = appPath;
    params["app_name"] = appName;
    QString query = "SELECT * FROM applications WHERE app_path = :app_path AND app_name = :app_name";

    logQueryWithValues(query, params);
    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> ApplicationModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("Application found by Path and Name: %1").arg(appPath));
        return QSharedPointer<ApplicationModel>(*result);
    } else {
        LOG_WARNING(QString("Application not found with Path and Name: %1").arg(appPath));
        return nullptr;
    }
}

QList<QSharedPointer<ApplicationModel>> ApplicationRepository::getByRoleId(const QUuid &roleId)
{
    LOG_DEBUG(QString("Get Application by RoleId: %1").arg(roleId.toString()));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<ApplicationModel>>();
    }

    QMap<QString, QVariant> params;
    params["role_id"] = roleId.toString(QUuid::WithoutBraces);
    QString query =
        "SELECT a.* FROM applications a "
        "JOIN tracked_applications_roles tar ON a.id = tar.app_id "
        "WHERE tar.role_id = :role_id "
        "ORDER BY a.app_name";

    auto applications = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> ApplicationModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<ApplicationModel>> result;
    for (auto app : applications) {
        result.append(QSharedPointer<ApplicationModel>(app));
    }

    LOG_INFO(QString("Retrieved %1 applications").arg(result.size()));
    return result;
}

QList<QSharedPointer<ApplicationModel>> ApplicationRepository::getByDisciplineId(const QUuid &disciplineId)
{
    LOG_DEBUG(QString("Get Application by disciplineId: %1").arg(disciplineId.toString()));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<ApplicationModel>>();
    }

    QMap<QString, QVariant> params;
    params["discipline_id"] = disciplineId.toString(QUuid::WithoutBraces);
    QString query =
        "SELECT a.* FROM applications a "
        "JOIN tracked_applications_disciplines tad ON a.id = tad.id "
        "WHERE tad.discipline_id = :discipline_id "
        "ORDER BY a.app_name";

    auto applications = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> ApplicationModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<ApplicationModel>> result;
    for (auto app : applications) {
        result.append(QSharedPointer<ApplicationModel>(app));
    }

    LOG_INFO(QString("Retrieved %1 applications").arg(result.size()));
    return result;
}

QList<QSharedPointer<ApplicationModel>> ApplicationRepository::getTrackedApplications()
{
    LOG_DEBUG(QString("Get Tracked Applications"));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<ApplicationModel>>();
    }

    QMap<QString, QVariant> params;
    QString query = "SELECT * FROM applications WHERE tracking_enabled = true ORDER BY app_name";
    auto applications = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> ApplicationModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<ApplicationModel>> result;
    for (auto app : applications) {
        result.append(QSharedPointer<ApplicationModel>(app));
    }

    LOG_INFO(QString("Retrieved %1 applications").arg(result.size()));
    return result;
}

QList<QSharedPointer<ApplicationModel>> ApplicationRepository::getRestrictedApplications()
{
    LOG_DEBUG(QString("Get Restricted Applications"));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<ApplicationModel>>();
    }

    QMap<QString, QVariant> params;
    QString query = "SELECT * FROM applications WHERE is_restricted = true ORDER BY app_name";
    auto applications = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> ApplicationModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<ApplicationModel>> result;
    for (auto app : applications) {
        result.append(QSharedPointer<ApplicationModel>(app));
    }

    LOG_INFO(QString("Retrieved %1 applications").arg(result.size()));
    return result;
}

bool ApplicationRepository::assignApplicationToRole(const QUuid &appId, const QUuid &roleId, const QUuid &userId)
{
    LOG_DEBUG(QString("Assigning Application %1 to Role %2").arg(appId.toString(), roleId.toString()));

    if (!ensureInitialized()) {
        return false;
    }

    // First check if the assignment already exists
    QMap<QString, QVariant> checkParams;
    checkParams["app_id"] = appId.toString(QUuid::WithoutBraces);
    checkParams["role_id"] = roleId.toString(QUuid::WithoutBraces);

    // Todo: Check query again if it fails with app_id
    QString checkQuery =
        "SELECT 1 FROM tracked_applications_roles "
        "WHERE app_id = :app_id AND role_id = :role_id";

    logQueryWithValues(checkQuery, checkParams);
    auto exists = m_dbService->executeSingleSelectQuery(
        checkQuery,
        checkParams,
        [this](const QSqlQuery& query) -> ApplicationModel* {
            return createModelFromQuery(query);
        }
    );

    if (exists) {
        LOG_INFO(QString("Application %1 already assigned to Role %2")
                .arg(appId.toString(), roleId.toString()));
        return true;
    }

    // Create the assignment
    QMap<QString, QVariant> params;
    params["app_id"] = appId.toString(QUuid::WithoutBraces);
    params["role_id"] = roleId.toString(QUuid::WithoutBraces);
    params["created_by"] = userId.toString(QUuid::WithoutBraces);
    params["created_at"] = QDateTime::currentDateTimeUtc().toUTC();
    params["updated_by"] = userId.toString(QUuid::WithoutBraces);
    params["updated_at"] = QDateTime::currentDateTimeUtc().toUTC();

    QString query =
        "INSERT INTO tracked_applications_roles "
        "(app_id, role_id, created_by, created_at, updated_by, updated_at) "
        "VALUES "
        "(:app_id, :role_id, :created_by, :created_at, :updated_by, :updated_at)";

    logQueryWithValues(query, params);
    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("Application %1 assigned to Role %2 successfully")
                .arg(appId.toString(), roleId.toString()));
    } else {
        LOG_ERROR(QString("Failed to assign Application %1 to Role %2: %3")
                .arg(appId.toString(), roleId.toString(), m_dbService->lastError()));
    }

    return success;
}

bool ApplicationRepository::removeApplicationFromRole(const QUuid &appId, const QUuid &roleId)
{
    LOG_DEBUG(QString("Removing Application %1 from Role %2").arg(appId.toString(), roleId.toString()));

    if (!ensureInitialized()) {
        return false;
    }

    QMap<QString, QVariant> params;
    params["app_id"] = appId.toString(QUuid::WithoutBraces);
    params["role_id"] = roleId.toString(QUuid::WithoutBraces);

    QString query =
        "DELETE FROM tracked_applications_roles "
        "WHERE app_id = :app_id AND role_id = :role_id";

    logQueryWithValues(query, params);
    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("Application %1 removed from Role %2 successfully")
                .arg(appId.toString(), roleId.toString()));
    } else {
        LOG_ERROR(QString("Failed to remove Application %1 from Role %2: %3")
                .arg(appId.toString(), roleId.toString(), m_dbService->lastError()));
    }

    return success;
}

bool ApplicationRepository::assignApplicationToDiscipline(const QUuid &appId, const QUuid &disciplineId, const QUuid &userId)
{
    LOG_DEBUG(QString("Assigning Application %1 to Discipline %2").arg(appId.toString(), disciplineId.toString()));

    if (!ensureInitialized()) {
        return false;
    }

    // First check if the assignment already exists
    QMap<QString, QVariant> checkParams;
    checkParams["app_id"] = appId.toString(QUuid::WithoutBraces);
    checkParams["discipline_id"] = disciplineId.toString(QUuid::WithoutBraces);

    QString checkQuery =
        "SELECT 1 FROM tracked_applications_disciplines "
        "WHERE app_id = :app_id AND discipline_id = :discipline_id";

    logQueryWithValues(checkQuery, checkParams);
    auto result = m_dbService->executeSingleSelectQuery(
        checkQuery,
        checkParams,
        [this](const QSqlQuery& query) -> ApplicationModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("Application %1 already assigned to Discipline %2")
                .arg(appId.toString(), disciplineId.toString()));
        return true;
    }

    // Create the assignment
    QMap<QString, QVariant> params;
    params["app_id"] = appId.toString(QUuid::WithoutBraces);
    params["discipline_id"] = disciplineId.toString(QUuid::WithoutBraces);
    params["created_by"] = userId.toString(QUuid::WithoutBraces);
    params["created_at"] = QDateTime::currentDateTimeUtc().toUTC();
    params["updated_by"] = userId.toString(QUuid::WithoutBraces);
    params["updated_at"] = QDateTime::currentDateTimeUtc().toUTC();

    QString query =
        "INSERT INTO tracked_applications_disciplines "
        "(app_id, discipline_id, created_by, created_at, updated_by, updated_at) "
        "VALUES "
        "(:app_id, :discipline_id, :created_by, :created_at, :updated_by, :updated_at)";

    logQueryWithValues(query, params);
    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("Application %1 assigned to Discipline %2 successfully")
                .arg(appId.toString(), disciplineId.toString()));
    } else {
        LOG_ERROR(QString("Failed to assign Application %1 to Discipline %2: %3")
                .arg(appId.toString(), disciplineId.toString(), m_dbService->lastError()));
    }

    return success;
}

bool ApplicationRepository::removeApplicationFromDiscipline(const QUuid &appId, const QUuid &disciplineId)
{
    LOG_DEBUG(QString("Removing Application %1 from Discipline %2").arg(appId.toString(), disciplineId.toString()));

    if (!ensureInitialized()) {
        return false;
    }

    QMap<QString, QVariant> params;
    params["app_id"] = appId.toString(QUuid::WithoutBraces);
    params["discipline_id"] = disciplineId.toString(QUuid::WithoutBraces);

    QString query =
        "DELETE FROM tracked_applications_disciplines "
        "WHERE app_id = :app_id AND discipline_id = :discipline_id";

    logQueryWithValues(query, params);
    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("Application %1 removed from Discipline %2 successfully")
                .arg(appId.toString(), disciplineId.toString()));
    } else {
        LOG_ERROR(QString("Failed to remove Application %1 from Discipline %2: %3")
                .arg(appId.toString(), disciplineId.toString(), m_dbService->lastError()));
    }

    return success;
}

QSharedPointer<ApplicationModel> ApplicationRepository::findOrCreateApplication(
    const QString &appName,
    const QString &appPath,
    const QString &appHash,
    bool isRestricted,
    bool trackingEnabled,
    const QUuid &createdBy)
{
    LOG_DEBUG(QString("Finding or creating application: %1 at %2").arg(appName, appPath));

    if (!ensureInitialized()) {
        return nullptr;
    }

    // First try to find by path and name
    auto app = getByPathAndName(appPath, appName);
    if (app) {
        LOG_INFO(QString("Found existing application by path and name: %1").arg(appName));
        return app;
    }

    // If not found, try by path only
    app = getByPath(appPath);
    if (app) {
        // Update the app name if it has changed
        if (app->appName() != appName) {
            LOG_INFO(QString("Updating application name from '%1' to '%2'")
                    .arg(app->appName(), appName));

            app->setAppName(appName);
            app->setUpdatedAt(QDateTime::currentDateTimeUtc());
            app->setUpdatedBy(createdBy);

            if (!update(app.data())) {
                LOG_ERROR(QString("Failed to update application name: %1").arg(m_dbService->lastError()));
                return nullptr;
            }
        }
        return app;
    }

    // Create a new application
    LOG_INFO(QString("Creating new application: %1").arg(appName));

    ApplicationModel *newApp = ModelFactory::createDefaultApplication(appName, appPath);
    newApp->setAppHash(appHash);
    newApp->setIsRestricted(isRestricted);
    newApp->setTrackingEnabled(trackingEnabled);
    newApp->setCreatedBy(createdBy);
    newApp->setUpdatedBy(createdBy);

    if (save(newApp)) {
        LOG_INFO(QString("Application created successfully: %1").arg(appName));
        return QSharedPointer<ApplicationModel>(newApp);
    }

    LOG_ERROR(QString("Failed to create application %1: %2")
            .arg(appName, m_dbService->lastError()));
    delete newApp;
    return nullptr;
}

void ApplicationRepository::logQueryWithValues(const QString& query, const QMap<QString, QVariant>& params)
{
    LOG_DEBUG("Executing query: " + query);

    if (!params.isEmpty()) {
        LOG_DEBUG("Query parameters:");
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            QString paramValue;

            if (it.value().isNull() || !it.value().isValid()) {
                paramValue = "NULL";
            } else if (it.value().type() == QVariant::String) {
                paramValue = "'" + it.value().toString() + "'";
            } else {
                paramValue = it.value().toString();
            }

            LOG_DEBUG(QString("  %1 = %2").arg(it.key(), paramValue));
        }
    }

    // For complex debuging, construct the actual query with values
    QString resolvedQuery = query;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        QString placeholder = ":" + it.key();
        QString value;

        if (it.value().isNull() || !it.value().isValid()) {
            value = "NULL";
        } else if (it.value().type() == QVariant::String) {
            value = "'" + it.value().toString() + "'";
        } else {
            value = it.value().toString();
        }

        resolvedQuery.replace(placeholder, value);
    }

    LOG_DEBUG("Resolved query: " + resolvedQuery);
}