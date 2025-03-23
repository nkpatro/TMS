#include "UserRoleDisciplineRepository.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>

#include "UserRepository.h"
#include "Core/ModelFactory.h"

UserRoleDisciplineRepository::UserRoleDisciplineRepository(QObject *parent)
    : BaseRepository<UserRoleDisciplineModel>(parent),
      m_userRepository(nullptr),
      m_roleRepository(nullptr),
      m_disciplineRepository(nullptr)
{
    LOG_DEBUG("UserRoleDisciplineRepository created");
}

void UserRoleDisciplineRepository::setUserRepository(UserRepository* userRepository) {
    m_userRepository = userRepository;
}

void UserRoleDisciplineRepository::setRoleRepository(RoleRepository* roleRepository) {
    m_roleRepository = roleRepository;
}

void UserRoleDisciplineRepository::setDisciplineRepository(DisciplineRepository* disciplineRepository) {
    m_disciplineRepository = disciplineRepository;
}

QString UserRoleDisciplineRepository::getEntityName() const
{
    return "UserRoleDiscipline";
}

QString UserRoleDisciplineRepository::getTableName() const
{
    return "user_role_disciplines";
}

QString UserRoleDisciplineRepository::getModelId(UserRoleDisciplineModel* model) const
{
    return model->id().toString();
}

QString UserRoleDisciplineRepository::buildSaveQuery()
{
    return "INSERT INTO user_role_disciplines "
           "(id, user_id, role_id, discipline_id, created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:id, :user_id, :role_id, :discipline_id, :created_at, :created_by, :updated_at, :updated_by) "
           "ON CONFLICT (user_id, role_id, discipline_id) DO UPDATE SET "
           "updated_at = :updated_at, "
           "updated_by = :updated_by";
}

QString UserRoleDisciplineRepository::buildUpdateQuery()
{
    return "UPDATE user_role_disciplines SET "
           "user_id = :user_id, "
           "role_id = :role_id, "
           "discipline_id = :discipline_id, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE id = :id";
}

QString UserRoleDisciplineRepository::buildGetByIdQuery()
{
    return "SELECT * FROM user_role_disciplines WHERE id = :id";
}

QString UserRoleDisciplineRepository::buildGetAllQuery()
{
    return "SELECT * FROM user_role_disciplines";
}

QString UserRoleDisciplineRepository::buildRemoveQuery()
{
    return "DELETE FROM user_role_disciplines WHERE id = :id";
}

QMap<QString, QVariant> UserRoleDisciplineRepository::prepareParamsForSave(UserRoleDisciplineModel* model)
{
    QMap<QString, QVariant> params;
    params["id"] = model->id().toString(QUuid::WithoutBraces);
    params["user_id"] = model->userId().toString(QUuid::WithoutBraces);
    params["role_id"] = model->roleId().toString(QUuid::WithoutBraces);
    params["discipline_id"] = model->disciplineId().toString(QUuid::WithoutBraces);
    params["created_at"] = model->createdAt().toString(Qt::ISODate);
    params["updated_at"] = model->updatedAt().toString(Qt::ISODate);

    // Handle nullable fields
    params["created_by"] = model->createdBy().isNull() ? QVariant(QVariant::Invalid) : model->createdBy().toString(QUuid::WithoutBraces);
    params["updated_by"] = model->updatedBy().isNull() ? QVariant(QVariant::Invalid) : model->updatedBy().toString(QUuid::WithoutBraces);
    
    return params;
}

QMap<QString, QVariant> UserRoleDisciplineRepository::prepareParamsForUpdate(UserRoleDisciplineModel* model)
{
    QMap<QString, QVariant> params;
    params["id"] = model->id().toString(QUuid::WithoutBraces);
    params["user_id"] = model->userId().toString(QUuid::WithoutBraces);
    params["role_id"] = model->roleId().toString(QUuid::WithoutBraces);
    params["discipline_id"] = model->disciplineId().toString(QUuid::WithoutBraces);
    params["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    // Handle nullable fields
    params["updated_by"] = model->updatedBy().isNull() ? QVariant(QVariant::Invalid) : model->updatedBy().toString(QUuid::WithoutBraces);
    
    return params;
}

UserRoleDisciplineModel* UserRoleDisciplineRepository::createModelFromQuery(const QSqlQuery &query)
{
    // Use ModelFactory to create the model
    return ModelFactory::createUserRoleDisciplineFromQuery(query);
}

bool UserRoleDisciplineRepository::validateModel(UserRoleDisciplineModel* model, QStringList& errors)
{
    return ModelFactory::validateUserRoleDisciplineModel(model, errors);
}

QList<QSharedPointer<UserRoleDisciplineModel>> UserRoleDisciplineRepository::getByUserId(const QUuid &userId)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot get user-role-disciplines by user ID: Repository not initialized");
        return QList<QSharedPointer<UserRoleDisciplineModel>>();
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM user_role_disciplines WHERE user_id = :user_id";

    auto models = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> UserRoleDisciplineModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<UserRoleDisciplineModel>> result;
    for (auto model : models) {
        result.append(QSharedPointer<UserRoleDisciplineModel>(model));
    }

    LOG_INFO(QString("Retrieved %1 user-role-discipline records for user %2")
             .arg(result.size())
             .arg(userId.toString()));
    return result;
}

QList<QSharedPointer<UserRoleDisciplineModel>> UserRoleDisciplineRepository::getByRoleId(const QUuid &roleId)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot get user-role-disciplines by role ID: Repository not initialized");
        return QList<QSharedPointer<UserRoleDisciplineModel>>();
    }

    QMap<QString, QVariant> params;
    params["role_id"] = roleId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM user_role_disciplines WHERE role_id = :role_id";

    auto models = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> UserRoleDisciplineModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<UserRoleDisciplineModel>> result;
    for (auto model : models) {
        result.append(QSharedPointer<UserRoleDisciplineModel>(model));
    }

    LOG_INFO(QString("Retrieved %1 user-role-discipline records for role %2")
             .arg(result.size())
             .arg(roleId.toString()));
    return result;
}

QList<QSharedPointer<UserRoleDisciplineModel>> UserRoleDisciplineRepository::getByDisciplineId(const QUuid &disciplineId)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot get user-role-disciplines by discipline ID: Repository not initialized");
        return QList<QSharedPointer<UserRoleDisciplineModel>>();
    }

    QMap<QString, QVariant> params;
    params["discipline_id"] = disciplineId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM user_role_disciplines WHERE discipline_id = :discipline_id";

    auto models = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> UserRoleDisciplineModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<UserRoleDisciplineModel>> result;
    for (auto model : models) {
        result.append(QSharedPointer<UserRoleDisciplineModel>(model));
    }

    LOG_INFO(QString("Retrieved %1 user-role-discipline records for discipline %2")
             .arg(result.size())
             .arg(disciplineId.toString()));
    return result;
}

QSharedPointer<UserRoleDisciplineModel> UserRoleDisciplineRepository::getByUserAndDiscipline(const QUuid &userId, const QUuid &disciplineId)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot get user-role-discipline by user and discipline: Repository not initialized");
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["discipline_id"] = disciplineId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM user_role_disciplines WHERE user_id = :user_id AND discipline_id = :discipline_id";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> UserRoleDisciplineModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("Found user-role-discipline record for user %1 and discipline %2")
                 .arg(userId.toString())
                 .arg(disciplineId.toString()));
        return QSharedPointer<UserRoleDisciplineModel>(*result);
    }

    LOG_INFO(QString("No user-role-discipline record found for user %1 and discipline %2")
             .arg(userId.toString())
             .arg(disciplineId.toString()));
    return nullptr;
}

bool UserRoleDisciplineRepository::userHasRoleInDiscipline(const QUuid &userId, const QUuid &roleId, const QUuid &disciplineId)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot check user-role-discipline: Repository not initialized");
        return false;
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["role_id"] = roleId.toString(QUuid::WithoutBraces);
    params["discipline_id"] = disciplineId.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT id FROM user_role_disciplines "
        "WHERE user_id = :user_id AND role_id = :role_id AND discipline_id = :discipline_id "
        "LIMIT 1";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> UserRoleDisciplineModel* {
            return createModelFromQuery(query);
        }
    );

    bool hasRole = (result != nullptr);

    if (result) {
        delete *result;
    }

    LOG_DEBUG(QString("User %1 %2 role %3 in discipline %4")
              .arg(userId.toString())
              .arg(hasRole ? "has" : "does not have")
              .arg(roleId.toString())
              .arg(disciplineId.toString()));
    
    return hasRole;
}

QList<QSharedPointer<RoleModel>> UserRoleDisciplineRepository::getRolesForUser(const QUuid &userId)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot get roles for user: Repository not initialized");
        return QList<QSharedPointer<RoleModel>>();
    }

    if (!m_roleRepository || !m_roleRepository->isInitialized()) {
        LOG_ERROR("RoleRepository not initialized, cannot get roles for user");
        return QList<QSharedPointer<RoleModel>>();
    }

    // Get role IDs first using our repository's database service
    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT DISTINCT role_id FROM user_role_disciplines "
        "WHERE user_id = :user_id";

    auto urdModels = m_dbService->executeSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> UserRoleDisciplineModel* {
            UserRoleDisciplineModel* model = new UserRoleDisciplineModel();
            if (!query.value("role_id").isNull()) {
                model->setRoleId(QUuid(query.value("role_id").toString()));
            }
            return model;
        }
    );

    // Extract role IDs
    QList<QUuid> roleIds;
    for (auto model : urdModels) {
        roleIds.append(model->roleId());
        delete model;
    }

    // Now get role info using the RoleRepository
    QList<QSharedPointer<RoleModel>> result;

    for (const QUuid& roleId : roleIds) {
        auto role = m_roleRepository->getById(roleId);
        if (role) {
            result.append(role);
        }
    }

    LOG_INFO(QString("Retrieved %1 roles for user %2")
             .arg(result.size())
             .arg(userId.toString()));
    
    return result;
}

QList<QSharedPointer<DisciplineModel>> UserRoleDisciplineRepository::getDisciplinesForUser(const QUuid &userId)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot get disciplines for user: Repository not initialized");
        return QList<QSharedPointer<DisciplineModel>>();
    }

    if (!m_disciplineRepository || !m_disciplineRepository->isInitialized()) {
        LOG_ERROR("DisciplineRepository not initialized, cannot get disciplines for user");
        return QList<QSharedPointer<DisciplineModel>>();
    }

    // Get discipline IDs first using our repository's database service
    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT DISTINCT discipline_id FROM user_role_disciplines "
        "WHERE user_id = :user_id";

    auto urdModels = m_dbService->executeSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> UserRoleDisciplineModel* {
            UserRoleDisciplineModel* model = new UserRoleDisciplineModel();
            if (!query.value("discipline_id").isNull()) {
                model->setDisciplineId(QUuid(query.value("discipline_id").toString()));
            }
            return model;
        }
    );

    // Extract discipline IDs
    QList<QUuid> disciplineIds;
    for (auto model : urdModels) {
        disciplineIds.append(model->disciplineId());
        delete model;
    }

    // Now get discipline info using the DisciplineRepository
    QList<QSharedPointer<DisciplineModel>> result;

    for (const QUuid& disciplineId : disciplineIds) {
        auto discipline = m_disciplineRepository->getById(disciplineId);
        if (discipline) {
            result.append(discipline);
        }
    }

    LOG_INFO(QString("Retrieved %1 disciplines for user %2")
             .arg(result.size())
             .arg(userId.toString()));
    
    return result;
}

QList<QSharedPointer<UserModel>> UserRoleDisciplineRepository::getUsersForRoleInDiscipline(const QUuid &roleId, const QUuid &disciplineId)
{
    if (!isInitialized()) {
        LOG_ERROR("Cannot get users for role in discipline: Repository not initialized");
        return QList<QSharedPointer<UserModel>>();
    }

    if (!m_userRepository || !m_userRepository->isInitialized()) {
        LOG_ERROR("UserRepository not initialized, cannot get users for role in discipline");
        return QList<QSharedPointer<UserModel>>();
    }

    // Get user IDs first using our repository's database service
    QMap<QString, QVariant> params;
    params["role_id"] = roleId.toString(QUuid::WithoutBraces);
    params["discipline_id"] = disciplineId.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT DISTINCT urd.user_id FROM user_role_disciplines urd "
        "WHERE urd.role_id = :role_id AND urd.discipline_id = :discipline_id";

    // Execute query to get user IDs
    auto urdModels = m_dbService->executeSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> UserRoleDisciplineModel* {
            UserRoleDisciplineModel* model = new UserRoleDisciplineModel();
            if (!query.value("user_id").isNull()) {
                model->setUserId(QUuid(query.value("user_id").toString()));
            }
            return model;
        }
    );

    // Extract user IDs
    QList<QUuid> userIds;
    for (auto model : urdModels) {
        userIds.append(model->userId());
        delete model;
    }

    // Now get user info using the UserRepository
    QList<QSharedPointer<UserModel>> result;

    for (const QUuid& userId : userIds) {
        auto user = m_userRepository->getById(userId);
        if (user) {
            result.append(user);
        }
    }

    LOG_INFO(QString("Retrieved %1 users for role %2 in discipline %3")
             .arg(result.size())
             .arg(roleId.toString())
             .arg(disciplineId.toString()));

    return result;
}

