#include "UserRepository.h"
#include <QDateTime>
#include <QSqlError>
#include "Core/ModelFactory.h"

UserRepository::UserRepository(QObject *parent)
    : BaseRepository<UserModel>(parent)
{
    LOG_DEBUG("UserRepository created");
}

QString UserRepository::getEntityName() const
{
    return "User";
}

QString UserRepository::getModelId(UserModel* model) const
{
    return model->id().toString();
}

QString UserRepository::buildSaveQuery()
{
    return "INSERT INTO users "
           "(name, email, password, photo, active, verified, verification_code, status_id, "
           "created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:name, :email, :password, :photo, :active::boolean, :verified::boolean, :verification_code, "
           ":status_id, :created_at, :created_by, :updated_at, :updated_by) "
           "RETURNING id";
}

QString UserRepository::buildUpdateQuery()
{
    return "UPDATE users SET "
           "name = :name, "
           "email = :email, "
           "photo = :photo, "
           "active = :active::boolean, "
           "verified = :verified::boolean, "
           "verification_code = :verification_code, "
           "status_id = :status_id, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE id = :id";
}

QString UserRepository::buildGetByIdQuery()
{
    return "SELECT * FROM users WHERE id = :id";
}

QString UserRepository::buildGetAllQuery()
{
    return "SELECT * FROM users ORDER BY name";
}

QString UserRepository::buildRemoveQuery()
{
    return "DELETE FROM users WHERE id = :id";
}

QMap<QString, QVariant> UserRepository::prepareParamsForSave(UserModel* user)
{
    QMap<QString, QVariant> params;
    params["name"] = user->name();
    params["email"] = user->email();
    params["password"] = hashPassword(user->password());
    params["photo"] = user->photo();
    params["active"] = user->active() ? "true" : "false";
    params["verified"] = user->verified() ? "true" : "false";
    params["verification_code"] = user->verificationCode();
    params["status_id"] = user->statusId().isNull() ? QVariant(QVariant::Invalid) : user->statusId().toString(QUuid::WithoutBraces);
    params["created_by"] = user->createdBy().isNull() ? QVariant(QVariant::Invalid) : user->createdBy().toString(QUuid::WithoutBraces);
    params["created_at"] = user->createdAt().toString(Qt::ISODate);
    params["updated_by"] = user->updatedBy().isNull() ? QVariant(QVariant::Invalid) : user->updatedBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = user->updatedAt().toString(Qt::ISODate);

    return params;
}

QMap<QString, QVariant> UserRepository::prepareParamsForUpdate(UserModel* user)
{
    QMap<QString, QVariant> params;
    params["id"] = user->id().toString(QUuid::WithoutBraces);
    params["name"] = user->name();
    params["email"] = user->email();
    params["photo"] = user->photo();
    params["active"] = user->active() ? "true" : "false";
    params["verified"] = user->verified() ? "true" : "false";
    params["verification_code"] = user->verificationCode();
    params["status_id"] = user->statusId().isNull() ? QVariant(QVariant::Invalid) : user->statusId().toString(QUuid::WithoutBraces);
    params["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    params["updated_by"] = user->updatedBy().isNull() ? QVariant(QVariant::Invalid) : user->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

UserModel* UserRepository::createModelFromQuery(const QSqlQuery &query)
{
    // Use ModelFactory to create the model
    return ModelFactory::createUserFromQuery(query);
}

bool UserRepository::save(UserModel *user)
{
    LOG_DEBUG(QString("Saving user: %1").arg(user->name()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot save user: Repository not initialized");
        return false;
    }

    return BaseRepository<UserModel>::save(user);
}

bool UserRepository::update(UserModel *user)
{
    LOG_DEBUG(QString("Updating user: %1 (%2)").arg(user->name(), user->id().toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot update user: Repository not initialized");
        return false;
    }

    return BaseRepository<UserModel>::update(user);
}

QSharedPointer<UserModel> UserRepository::getByName(const QString &name)
{
    LOG_DEBUG(QString("Getting user by name: %1").arg(name));

    if (!isInitialized()) {
        LOG_ERROR(QString("Cannot get user by name: Repository not initialized"));
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["name"] = name;

    QString query = "SELECT * FROM users WHERE name = :name";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> UserModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("User found: %1 (%2)").arg((*result)->name(), (*result)->id().toString()));
        return QSharedPointer<UserModel>(*result);
    } else {
        LOG_WARNING(QString("User not found with name: %1").arg(name));
        return nullptr;
    }
}

QSharedPointer<UserModel> UserRepository::getByEmail(const QString &email)
{
    LOG_DEBUG(QString("Getting user by email: %1").arg(email));

    if (!isInitialized()) {
        LOG_ERROR(QString("Cannot get user by email: Repository not initialized"));
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["email"] = email;

    QString query = "SELECT * FROM users WHERE email = :email";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> UserModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("User found: %1 (%2)").arg((*result)->name(), (*result)->id().toString()));
        return QSharedPointer<UserModel>(*result);
    } else {
        LOG_WARNING(QString("User not found with email: %1").arg(email));
        return nullptr;
    }
}

QList<QSharedPointer<UserModel>> UserRepository::getActiveUsers()
{
    LOG_DEBUG("Getting active users");

    if (!isInitialized()) {
        LOG_ERROR("Cannot get active users: Repository not initialized");
        return QList<QSharedPointer<UserModel>>();
    }

    QString query = "SELECT * FROM users WHERE active = true ORDER BY name";
    QMap<QString, QVariant> params;

    QVector<UserModel*> users = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> UserModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<UserModel>> result;
    for (auto user : users) {
        result.append(QSharedPointer<UserModel>(user));
    }

    LOG_INFO(QString("Retrieved %1 active users").arg(result.size()));
    return result;
}

bool UserRepository::setUserActive(const QUuid &id, bool active)
{
    LOG_DEBUG(QString("Setting user %1 active status to: %2").arg(id.toString()).arg(active));

    if (!isInitialized()) {
        LOG_ERROR("Cannot set user active status: Repository not initialized");
        return false;
    }

    QMap<QString, QVariant> params;
    params["id"] = id.toString(QUuid::WithoutBraces);
    params["active"] = active ? "true" : "false";
    params["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QString query =
        "UPDATE users SET "
        "active = :active::boolean, "
        "updated_at = :updated_at "
        "WHERE id = :id";

    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("User active status updated successfully: %1 -> %2").arg(id.toString()).arg(active));
    } else {
        LOG_ERROR(QString("Failed to update user active status: %1 -> %2 - %3")
                .arg(id.toString()).arg(active).arg(m_dbService->lastError()));
    }

    return success;
}

bool UserRepository::setUserVerified(const QUuid &id, bool verified)
{
    LOG_DEBUG(QString("Setting user %1 verified status to: %2").arg(id.toString()).arg(verified));

    if (!isInitialized()) {
        LOG_ERROR("Cannot set user verified status: Repository not initialized");
        return false;
    }

    QMap<QString, QVariant> params;
    params["id"] = id.toString(QUuid::WithoutBraces);
    params["verified"] = verified ? "true" : "false";
    params["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QString query =
        "UPDATE users SET "
        "verified = :verified::boolean, "
        "updated_at = :updated_at "
        "WHERE id = :id";

    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("User verified status updated successfully: %1 -> %2").arg(id.toString()).arg(verified));
    } else {
        LOG_ERROR(QString("Failed to update user verified status: %1 -> %2 - %3")
                .arg(id.toString()).arg(verified).arg(m_dbService->lastError()));
    }

    return success;
}

bool UserRepository::validateCredentials(const QString &email, const QString &password, QSharedPointer<UserModel> &user)
{
    LOG_DEBUG(QString("Validating credentials for email: %1").arg(email));

    if (!isInitialized()) {
        LOG_ERROR("Cannot validate credentials: Repository not initialized");
        return false;
    }

    QMap<QString, QVariant> params;
    params["email"] = email;
    params["password"] = hashPassword(password);

    QString query = "SELECT * FROM users WHERE email = :email AND password = :password";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> UserModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        user = QSharedPointer<UserModel>(*result);
        LOG_INFO(QString("Credentials validated successfully for user: %1 (%2)")
                .arg(user->name(), user->id().toString()));
        return true;
    } else {
        LOG_WARNING(QString("Invalid credentials for email: %1").arg(email));
        return false;
    }
}

bool UserRepository::updatePassword(const QUuid &id, const QString &newPassword)
{
    LOG_DEBUG(QString("Updating password for user: %1").arg(id.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot update password: Repository not initialized");
        return false;
    }

    QMap<QString, QVariant> params;
    params["id"] = id.toString(QUuid::WithoutBraces);
    params["password"] = hashPassword(newPassword);
    params["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QString query =
        "UPDATE users SET "
        "password = :password, "
        "updated_at = :updated_at "
        "WHERE id = :id";

    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("Password updated successfully for user: %1").arg(id.toString()));
    } else {
        LOG_ERROR(QString("Failed to update password for user: %1 - %2")
                .arg(id.toString(), m_dbService->lastError()));
    }

    return success;
}

QSharedPointer<UserModel> UserRepository::findOrCreateUser(
    const QString &name,
    const QString &email,
    const QString &password,
    const QUuid &createdBy)
{
    LOG_DEBUG(QString("Finding or creating user with email: %1").arg(email));

    if (!isInitialized()) {
        LOG_ERROR("Cannot find or create user: Repository not initialized");
        return nullptr;
    }

    // Check if user exists
    auto existingUser = getByEmail(email);
    if (existingUser) {
        LOG_INFO(QString("Found existing user: %1 (%2)")
                .arg(existingUser->name(), existingUser->id().toString()));
        return existingUser;
    }

    // Create new user
    UserModel *newUser = ModelFactory::createDefaultUser(name, email);
    newUser->setPassword(password);
    newUser->setCreatedBy(createdBy);
    newUser->setUpdatedBy(createdBy);

    if (save(newUser)) {
        LOG_INFO(QString("Created new user: %1 (%2)")
                .arg(newUser->name(), newUser->id().toString()));
        return QSharedPointer<UserModel>(newUser);
    } else {
        LOG_ERROR(QString("Failed to create new user: %1 <%2>")
                .arg(name, email));
        delete newUser;
        return nullptr;
    }
}

QString UserRepository::hashPassword(const QString &password)
{
    // In a real app, use a proper password hashing library like bcrypt
    // This is a simple implementation for demonstration purposes
    QByteArray hash = QCryptographicHash::hash(
        password.toUtf8(),
        QCryptographicHash::Sha256
    );

    return hash.toHex();
}