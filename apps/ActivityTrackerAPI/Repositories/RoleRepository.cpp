#include "RoleRepository.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QJsonObject>
#include "Core/ModelFactory.h"

// Helper class for role statistics
class RoleStatsResult : public RoleModel {
public:
    int totalUsers;
    int activeSessions;
    QDateTime lastUsed;

    RoleStatsResult() : RoleModel(),
        totalUsers(0),
        activeSessions(0) {}
};

RoleRepository::RoleRepository(QObject *parent)
    : BaseRepository<RoleModel>(parent)
{
    LOG_DEBUG("RoleRepository created");
}

QString RoleRepository::getEntityName() const
{
    return "Role";
}

QString RoleRepository::getModelId(RoleModel* model) const
{
    return model->id().toString();
}

QString RoleRepository::buildSaveQuery()
{
    return "INSERT INTO roles "
           "(code, name, description, created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:code, :name, :description, :created_at, :created_by, :updated_at, :updated_by)";
}

QString RoleRepository::buildUpdateQuery()
{
    return "UPDATE roles SET "
           "code = :code, "
           "name = :name, "
           "description = :description, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE id = :id";
}

QString RoleRepository::buildGetByIdQuery()
{
    return "SELECT * FROM roles WHERE id = :id";
}

QString RoleRepository::buildGetAllQuery()
{
    return "SELECT * FROM roles ORDER BY name";
}

QString RoleRepository::buildRemoveQuery()
{
    return "DELETE FROM roles WHERE id = :id";
}

QMap<QString, QVariant> RoleRepository::prepareParamsForSave(RoleModel* role)
{
    QMap<QString, QVariant> params;
    params["code"] = role->code();
    params["name"] = role->name();
    params["description"] = role->description();
    params["created_at"] = role->createdAt().toString(Qt::ISODate);
    params["created_by"] = role->createdBy().isNull() ? QVariant(QVariant::Invalid) : role->createdBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = role->updatedAt().toString(Qt::ISODate);
    params["updated_by"] = role->updatedBy().isNull() ? QVariant(QVariant::Invalid) : role->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

QMap<QString, QVariant> RoleRepository::prepareParamsForUpdate(RoleModel* role)
{
    QMap<QString, QVariant> params;
    params["id"] = role->id().toString(QUuid::WithoutBraces);
    params["code"] = role->code();
    params["name"] = role->name();
    params["description"] = role->description();
    params["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    params["updated_by"] = role->updatedBy().isNull() ? QVariant(QVariant::Invalid) : role->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

RoleModel* RoleRepository::createModelFromQuery(const QSqlQuery &query)
{
    return ModelFactory::createRoleFromQuery(query);
}

bool RoleRepository::remove(const QUuid &id)
{
    LOG_DEBUG(QString("Removing role: %1").arg(id.toString()));

    if (!ensureInitialized()) {
        return false;
    }

    // Begin transaction to ensure data integrity
    return executeInTransaction([this, id]() {
        // First check if this role is referenced in sessions
        QMap<QString, QVariant> checkParams;
        checkParams["role_id"] = id.toString(QUuid::WithoutBraces);

        QString checkQuery = "SELECT COUNT(*) as ref_count FROM sessions WHERE role_id = :role_id";

        LOG_DEBUG(QString("Checking for role references: %1").arg(checkQuery));
        LOG_DEBUG(QString("With parameters: role_id=%1").arg(checkParams["role_id"].toString()));

        auto checkResult = m_dbService->executeSingleSelectQuery(
            checkQuery,
            checkParams,
            [](const QSqlQuery& query) -> RoleModel* {
                // We're just using this to get the count
                RoleModel* model = ModelFactory::createDefaultRole();
                model->setDescription(query.value("ref_count").toString());
                return model;
            }
        );

        if (checkResult) {
            int refCount = (*checkResult)->description().toInt();
            delete *checkResult;

            if (refCount > 0) {
                LOG_WARNING(QString("Cannot remove role %1: Referenced in %2 sessions").arg(id.toString()).arg(refCount));
                return false;
            }
        }

        // Also check if referenced in user_roles
        QMap<QString, QVariant> userRoleParams;
        userRoleParams["role_id"] = id.toString(QUuid::WithoutBraces);

        QString userRoleQuery = "SELECT COUNT(*) as ref_count FROM user_roles WHERE role_id = :role_id";

        LOG_DEBUG(QString("Checking for user role references: %1").arg(userRoleQuery));
        LOG_DEBUG(QString("With parameters: role_id=%1").arg(userRoleParams["role_id"].toString()));

        auto userRoleResult = m_dbService->executeSingleSelectQuery(
            userRoleQuery,
            userRoleParams,
            [](const QSqlQuery& query) -> RoleModel* {
                RoleModel* model = ModelFactory::createDefaultRole();
                model->setDescription(query.value("ref_count").toString());
                return model;
            }
        );

        if (userRoleResult) {
            int userRoleCount = (*userRoleResult)->description().toInt();
            delete *userRoleResult;

            if (userRoleCount > 0) {
                LOG_WARNING(QString("Cannot remove role %1: Referenced in %2 user_roles records").arg(id.toString()).arg(userRoleCount));
                return false;
            }
        }

        // Proceed with deletion using base repository method
        QMap<QString, QVariant> params;
        params["id"] = id.toString(QUuid::WithoutBraces);

        QString query = buildRemoveQuery();

        LOG_DEBUG(QString("Executing remove query: %1").arg(query));
        LOG_DEBUG(QString("With parameters: id=%1").arg(params["id"].toString()));

        bool success = m_dbService->executeModificationQuery(query, params);

        if (success) {
            LOG_INFO(QString("Role removed successfully: %1").arg(id.toString()));
            return true;
        } else {
            LOG_ERROR(QString("Failed to remove role: %1 - %2")
                    .arg(id.toString(), m_dbService->lastError()));
            return false;
        }
    });
}

QSharedPointer<RoleModel> RoleRepository::getByCode(const QString &code)
{
    LOG_DEBUG(QString("Getting role by code: %1").arg(code));

    if (!ensureInitialized()) {
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["code"] = code;

    QString query = "SELECT * FROM roles WHERE code = :code";

    LOG_DEBUG(QString("Executing getByCode query: %1").arg(query));
    LOG_DEBUG(QString("With parameters: code=%1").arg(params["code"].toString()));

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> RoleModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("Role found by code: %1 (%2)").arg(code, (*result)->id().toString()));
        return QSharedPointer<RoleModel>(*result);
    } else {
        LOG_WARNING(QString("Role not found with code: %1").arg(code));
        return nullptr;
    }
}

QList<QSharedPointer<RoleModel>> RoleRepository::getByName(const QString &name)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot get roles by name: Repository not initialized");
        return QList<QSharedPointer<RoleModel>>();
    }

    QMap<QString, QVariant> params;
    // Change 'term' to 'name'
    params["name"] = "%" + name + "%";  // Instead of term

    QString query = "SELECT * FROM roles WHERE name ILIKE :name OR code ILIKE :name";

    auto roles = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> RoleModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<RoleModel>> result;
    for (auto role : roles) {
        result.append(QSharedPointer<RoleModel>(role));
    }

    LOG_INFO(QString("Retrieved %1 roles matching name: %2")
            .arg(result.size())
            .arg(name));  // Instead of term

    return result;
}

QJsonObject RoleRepository::getRoleStats(const QUuid &id)
{
    LOG_DEBUG(QString("Getting statistics for role: %1").arg(id.toString()));

    QJsonObject stats;

    if (!ensureInitialized()) {
        return stats;
    }

    QMap<QString, QVariant> params;
    params["role_id"] = id.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT "
        "COUNT(DISTINCT user_id) as total_users, "
        "COUNT(*) as total_sessions, "
        "COUNT(CASE WHEN logout_time IS NULL THEN 1 END) as active_sessions, "
        "MAX(login_time) as last_used "
        "FROM sessions "
        "WHERE role_id = :role_id";

    LOG_DEBUG(QString("Executing role stats query: %1").arg(query));
    LOG_DEBUG(QString("With parameters: role_id=%1").arg(params["role_id"].toString()));

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> RoleModel* {
            RoleStatsResult* statsResult = new RoleStatsResult();

            statsResult->totalUsers = query.value("total_users").toInt();
            statsResult->activeSessions = query.value("active_sessions").toInt();

            if (!query.value("last_used").isNull()) {
                statsResult->lastUsed = query.value("last_used").toDateTime();
            }

            QJsonObject data;
            data["total_users"] = statsResult->totalUsers;
            data["total_sessions"] = query.value("total_sessions").toInt();
            data["active_sessions"] = statsResult->activeSessions;

            if (statsResult->lastUsed.isValid()) {
                data["last_used"] = statsResult->lastUsed.toString(Qt::ISODate);
            } else {
                data["last_used"] = QJsonValue::Null;
            }

            // Store in model for easy extraction
            QJsonDocument doc(data);
            statsResult->setDescription(QString::fromUtf8(doc.toJson()));

            return statsResult;
        }
    );

    if (result) {
        RoleStatsResult* statsResult = static_cast<RoleStatsResult*>(*result);

        // Extract JSON from description where we stored it
        QJsonDocument doc = QJsonDocument::fromJson(statsResult->description().toUtf8());
        stats = doc.object();

        LOG_INFO(QString("Retrieved stats for role %1: %2 users, %3 active sessions")
                .arg(id.toString())
                .arg(statsResult->totalUsers)
                .arg(statsResult->activeSessions));

        delete statsResult;
    } else {
        LOG_WARNING(QString("Failed to get stats for role: %1").arg(id.toString()));
    }

    return stats;
}

bool RoleRepository::batchCreate(const QList<RoleModel*> &roles)
{
    LOG_DEBUG(QString("Batch creating %1 roles").arg(roles.size()));

    if (!ensureInitialized()) {
        return false;
    }

    if (roles.isEmpty()) {
        LOG_WARNING("No roles provided for batch creation");
        return true; // Nothing to do, so technically successful
    }

    // Use the executeInTransaction method from BaseRepository
    return executeInTransaction([this, &roles]() {
        int successCount = 0;

        for (RoleModel* role : roles) {
            if (save(role)) {
                successCount++;
            } else {
                LOG_ERROR(QString("Failed to save role in batch: %1 (%2)")
                        .arg(role->name(), role->id().toString()));
            }
        }

        // If at least one succeeded, consider the batch operation partially successful
        return successCount > 0;
    });
}

QSharedPointer<RoleModel> RoleRepository::getOrCreate(const QString &code, const QString &name, const QString &description)
{
    LOG_DEBUG(QString("Getting or creating role with code: %1").arg(code));

    if (!ensureInitialized()) {
        return nullptr;
    }

    // First try to get by code
    auto existingRole = getByCode(code);
    if (existingRole) {
        LOG_INFO(QString("Found existing role with code %1: %2")
                .arg(code, existingRole->name()));
        return existingRole;
    }

    // Need to create a new one
    LOG_DEBUG(QString("No role found with code %1, creating new one").arg(code));

    // Use ModelFactory to create the default role
    RoleModel* newRole = ModelFactory::createDefaultRole(name, code);
    newRole->setDescription(description);

    if (save(newRole)) {
        LOG_INFO(QString("Created new role with code %1: %2 (%3)")
                .arg(code, name, newRole->id().toString()));
        return QSharedPointer<RoleModel>(newRole);
    } else {
        LOG_ERROR(QString("Failed to create new role with code %1").arg(code));
        delete newRole;
        return nullptr;
    }
}

QList<QSharedPointer<RoleModel>> RoleRepository::searchRoles(const QString& term, int limit)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot search roles: Repository not initialized");
        return QList<QSharedPointer<RoleModel>>();
    }

    QMap<QString, QVariant> params;
    params["term"] = "%" + term + "%";
    params["limit"] = limit;

    QString query =
        "SELECT * FROM roles "
        "WHERE (name ILIKE :term OR code ILIKE :term OR description ILIKE :term) "
        "ORDER BY name "
        "LIMIT :limit";

    auto roles = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> RoleModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<RoleModel>> result;
    for (auto role : roles) {
        result.append(QSharedPointer<RoleModel>(role));
    }

    LOG_INFO(QString("Search found %1 roles matching term: %2")
            .arg(result.size())
            .arg(term));

    return result;
}

