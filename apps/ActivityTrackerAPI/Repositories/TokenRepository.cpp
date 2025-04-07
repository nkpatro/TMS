#include "TokenRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include "logger/logger.h"
#include "Core/ModelFactory.h"
#include "dbservice/dbmanager.h"

TokenRepository::TokenRepository(QObject *parent)
    : BaseRepository<TokenModel>(parent)
{
    LOG_DEBUG("TokenRepository instance created");
}

QString TokenRepository::getEntityName() const
{
    return "AuthToken";
}

QString TokenRepository::getTableName() const
{
    return "auth_tokens";
}

QString TokenRepository::getIdParamName() const
{
    return "token_id";
}

QString TokenRepository::getModelId(TokenModel* model) const
{
    return model->tokenId();
}

QString TokenRepository::buildSaveQuery()
{
    // Use JSONB type for PostgreSQL
    return "INSERT INTO auth_tokens "
           "(token_id, token_type, user_id, token_data, expires_at, created_at, "
           "created_by, updated_at, updated_by, device_info, last_used_at) "
           "VALUES "
           "(:token_id, :token_type, :user_id, :token_data::jsonb, :expires_at, :created_at, "
           ":created_by, :updated_at, :updated_by, :device_info::jsonb, :last_used_at) "
           "RETURNING token_id";
}

QString TokenRepository::buildUpdateQuery()
{
    return "UPDATE auth_tokens SET "
           "token_type = :token_type, "
           "user_id = :user_id, "
           "token_data = :token_data::jsonb, "
           "expires_at = :expires_at, "
           "device_info = :device_info::jsonb, "
           "revoked = :revoked::boolean, "
           "revocation_reason = :revocation_reason, "
           "last_used_at = :last_used_at, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE token_id = :token_id";
}

QString TokenRepository::buildGetByIdQuery()
{
    return "SELECT * FROM auth_tokens WHERE token_id = :id";
}

QString TokenRepository::buildGetAllQuery()
{
    return "SELECT * FROM auth_tokens ORDER BY created_at DESC";
}

QString TokenRepository::buildRemoveQuery()
{
    return "DELETE FROM auth_tokens WHERE token_id = :id";
}

QString TokenRepository::buildGetByTokenQuery()
{
    return "SELECT * FROM auth_tokens WHERE token_id = :token_id";
}

QString TokenRepository::buildGetActiveTokensQuery()
{
    return "SELECT * FROM auth_tokens "
           "WHERE revoked = false AND expires_at > CURRENT_TIMESTAMP";
}

QString TokenRepository::buildRevokeTokenQuery()
{
    return "UPDATE auth_tokens SET "
           "revoked = true, "
           "revocation_reason = :revocation_reason, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE token_id = :token_id";
}

QString TokenRepository::buildRevokeAllUserTokensQuery()
{
    return "UPDATE auth_tokens SET "
           "revoked = true, "
           "revocation_reason = :revocation_reason, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE user_id = :user_id AND revoked = false";
}

QString TokenRepository::buildUpdateLastUsedQuery()
{
    return "UPDATE auth_tokens SET last_used_at = :last_used_at, "
           "updated_at = :updated_at "
           "WHERE token_id = :token_id";
}

QMap<QString, QVariant> TokenRepository::prepareParamsForSave(TokenModel* token)
{
    QMap<QString, QVariant> params;
    params["id"] = token->id().toString(QUuid::WithoutBraces);
    params["token_id"] = token->tokenId();
    params["token_type"] = token->tokenType();
    params["user_id"] = token->userId().toString(QUuid::WithoutBraces);
    params["token_data"] = jsonToString(token->tokenData());
    params["expires_at"] = token->expiresAt().toUTC();
    params["created_at"] = token->createdAt().toUTC();
    params["created_by"] = token->createdBy().isNull() ? QVariant(QVariant::String) : token->createdBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = token->updatedAt().toUTC();
    params["updated_by"] = token->updatedBy().isNull() ? QVariant(QVariant::String) : token->updatedBy().toString(QUuid::WithoutBraces);
    params["device_info"] = jsonToString(token->deviceInfo());
    params["last_used_at"] = token->lastUsedAt().toUTC();

    // Log the parameter details for debugging
    LOG_DEBUG(QString("Prepared save parameters for token: %1").arg(token->tokenId()));

    return params;
}

QMap<QString, QVariant> TokenRepository::prepareParamsForUpdate(TokenModel* token)
{
    QMap<QString, QVariant> params = prepareParamsForSave(token);
    params["revoked"] = token->isRevoked() ? "true" : "false";
    params["revocation_reason"] = token->revocationReason();

    // Log the update action
    LOG_DEBUG(QString("Prepared update parameters for token: %1 (revoked: %2)")
              .arg(token->tokenId())
              .arg(token->isRevoked() ? "yes" : "no"));

    return params;
}

bool TokenRepository::validateModel(TokenModel* model, QStringList& errors)
{
    LOG_DEBUG(QString("Validating token model: %1").arg(model->tokenId()));

    // Validate required fields
    if (model->tokenId().isEmpty()) {
        errors.append("Token ID is required");
    }

    if (model->tokenType().isEmpty()) {
        errors.append("Token type is required");
    }

    if (model->userId().isNull()) {
        errors.append("User ID is required");
    }

    if (!model->expiresAt().isValid()) {
        errors.append("Expiration time is required and must be valid");
    }

    if (!model->createdAt().isValid()) {
        errors.append("Creation time is required and must be valid");
    }

    if (!errors.isEmpty()) {
        LOG_WARNING(QString("Token validation failed: %1").arg(errors.join(", ")));
    } else {
        LOG_DEBUG(QString("Token validation successful for: %1").arg(model->tokenId()));
    }

    return errors.isEmpty();
}

TokenModel* TokenRepository::createModelFromQuery(const QSqlQuery &query)
{
    // Use ModelFactory pattern for consistency instead of manual creation
    TokenModel* token = ModelFactory::createTokenFromQuery(query);

    LOG_DEBUG(QString("Created token model from query: %1 (type: %2)")
              .arg(token->tokenId(), token->tokenType()));

    return token;
}

bool TokenRepository::saveToken(const QString& token, const QString& tokenType,
                             const QUuid& userId, const QJsonObject& tokenData,
                             const QDateTime& expiryTime, const QUuid& createdBy)
{
    LOG_DEBUG(QString("Storing %1 token for user: %2").arg(tokenType, userId.toString()));

    if (!ensureInitialized()) {
        LOG_ERROR("Cannot store token: Repository not initialized");
        return false;
    }

    // Check if token already exists
    bool tokenExists = this->tokenExists(token);
    LOG_DEBUG(QString("Token exists check: %1 - %2").arg(token).arg(tokenExists ? "yes" : "no"));

    if (tokenExists) {
        // Update existing token
        auto existingToken = getById(QUuid(token));
        if (!existingToken) {
            LOG_ERROR(QString("Token exists but could not be retrieved: %1").arg(token));
            return false;
        }

        // Update token properties
        existingToken->setTokenType(tokenType);
        existingToken->setUserId(userId);
        existingToken->setTokenData(tokenData);
        existingToken->setExpiresAt(expiryTime);
        existingToken->setRevoked(false);
        existingToken->setRevocationReason(QString());
        existingToken->setLastUsedAt(QDateTime::currentDateTimeUtc());
        existingToken->setUpdatedAt(QDateTime::currentDateTimeUtc());

        if (!createdBy.isNull()) {
            existingToken->setUpdatedBy(createdBy);
        }

        // Use BaseRepository update method
        bool success = update(existingToken.data());

        if (success) {
            LOG_INFO(QString("Token updated successfully: %1").arg(token));
        } else {
            LOG_ERROR(QString("Failed to update token: %1 - %2").arg(token, lastError()));
        }

        return success;
    } else {
        // Create new token model using ModelFactory pattern
        TokenModel* tokenModel = ModelFactory::createDefaultToken(token, userId, tokenType);

        // Set token-specific properties
        tokenModel->setTokenData(tokenData);
        tokenModel->setExpiresAt(expiryTime);

        if (!createdBy.isNull()) {
            tokenModel->setCreatedBy(createdBy);
            tokenModel->setUpdatedBy(createdBy);
        }

        // Add device info if available in token data
        if (tokenData.contains("device_info") && tokenData["device_info"].isObject()) {
            tokenModel->setDeviceInfo(tokenData["device_info"].toObject());
        }

        // Log query parameters before execution
        logQueryWithValues(buildSaveQuery(), prepareParamsForSave(tokenModel));

        // Save using base repository method
        bool success = save(tokenModel);

        if (success) {
            LOG_INFO(QString("Token saved successfully: %1").arg(token));
        } else {
            LOG_ERROR(QString("Failed to save token: %1 - %2").arg(token, lastError()));
        }

        delete tokenModel;
        return success;
    }
}

bool TokenRepository::validateToken(const QString& token, QJsonObject& tokenData)
{
    LOG_DEBUG(QString("Validating token: %1").arg(token));

    if (!ensureInitialized()) {
        LOG_ERROR("Cannot validate token: Repository not initialized");
        return false;
    }

    QMap<QString, QVariant> params;
    params["token_id"] = token;

    QString query = buildGetByTokenQuery();

    // Log query before execution
    logQueryWithValues(query, params);

    auto result = executeSingleSelectQuery(query, params);

    if (result) {
        // Check if token is revoked
        if (result->isRevoked()) {
            LOG_WARNING(QString("Token is revoked: %1").arg(token));
            return false;
        }

        // Check if token is expired
        if (result->isExpired()) {
            LOG_WARNING(QString("Token has expired: %1").arg(token));
            return false;
        }

        // Update last used timestamp
        updateTokenLastUsed(token);

        // Extract token data
        tokenData = result->tokenData();
        LOG_DEBUG(QString("Token validated successfully: %1").arg(token));
        return true;
    }

    LOG_WARNING(QString("Token not found: %1").arg(token));
    return false;
}

bool TokenRepository::revokeToken(const QString& token, const QString& reason)
{
    LOG_DEBUG(QString("Revoking token: %1").arg(token));

    if (!ensureInitialized()) {
        LOG_ERROR("Cannot revoke token: Repository not initialized");
        return false;
    }

    auto existingToken = getById(QUuid(token));
    if (!existingToken) {
        LOG_ERROR(QString("Token not found for revocation: %1").arg(token));
        return false;
    }

    existingToken->setRevoked(true);
    existingToken->setRevocationReason(reason.isEmpty() ? "Manually revoked" : reason);
    existingToken->setUpdatedAt(QDateTime::currentDateTimeUtc());

    // Log query before execution
    logQueryWithValues(buildUpdateQuery(), prepareParamsForUpdate(existingToken.data()));

    bool success = update(existingToken.data());

    if (success) {
        LOG_INFO(QString("Token revoked successfully: %1 (Reason: %2)")
                .arg(token)
                .arg(existingToken->revocationReason()));
    } else {
        LOG_ERROR(QString("Failed to revoke token: %1 - %2").arg(token, lastError()));
    }

    return success;
}

bool TokenRepository::revokeAllUserTokens(const QUuid& userId, const QString& reason)
{
    LOG_DEBUG(QString("Revoking all tokens for user: %1").arg(userId.toString()));

    if (!ensureInitialized()) {
        LOG_ERROR("Cannot revoke user tokens: Repository not initialized");
        return false;
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["revocation_reason"] = reason.isEmpty() ? "User logout" : reason;
    params["updated_at"] = QDateTime::currentDateTimeUtc().toUTC();
    params["updated_by"] = QVariant(QVariant::String); // Optional, can be set if known

    QString query = buildRevokeAllUserTokensQuery();

    // Log query before execution
    logQueryWithValues(query, params);

    bool success = executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("All tokens revoked for user: %1 (Reason: %2)")
                .arg(userId.toString(), params["revocation_reason"].toString()));
    } else {
        LOG_ERROR(QString("Failed to revoke tokens for user: %1 - %2")
                 .arg(userId.toString(), lastError()));
    }

    return success;
}

bool TokenRepository::loadActiveTokens(QMap<QString, QJsonObject>& tokenMap)
{
    LOG_DEBUG("Loading active tokens from database");

    if (!ensureInitialized()) {
        LOG_ERROR("Cannot load tokens: Repository not initialized");
        return false;
    }

    tokenMap.clear();
    QString query = buildGetActiveTokensQuery();

    // Log query before execution
    logQueryWithValues(query, QMap<QString, QVariant>());

    QList<QSharedPointer<TokenModel>> tokens = executeSelectQuery(query, QMap<QString, QVariant>());

    int count = 0;
    for (const auto& token : tokens) {
        tokenMap[token->tokenId()] = token->tokenData();
        count++;
    }

    LOG_INFO(QString("Loaded %1 active tokens from database").arg(count));
    return true;
}

int TokenRepository::purgeExpiredTokens()
{
    LOG_DEBUG("Purging expired tokens");

    if (!ensureInitialized()) {
        LOG_ERROR("Cannot purge tokens: Repository not initialized");
        return 0;
    }

    // Use the dbservice directly for this specialized query that returns count
    QString query = "DELETE FROM auth_tokens WHERE expires_at < CURRENT_TIMESTAMP RETURNING token_id";

    // Log query before execution
    logQueryWithValues(query, QMap<QString, QVariant>());

    // Execute a custom query to get the count of deleted rows
    QSqlQuery sqlQuery = m_dbService->createQuery();
    sqlQuery.prepare(query);

    int count = 0;
    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            count++;
        }
        LOG_INFO(QString("Purged %1 expired tokens").arg(count));
    } else {
        LOG_ERROR(QString("Failed to purge expired tokens: %1").arg(sqlQuery.lastError().text()));
    }

    return count;
}

bool TokenRepository::updateTokenLastUsed(const QString& token)
{
    if (!ensureInitialized()) {
        LOG_ERROR(QString("Cannot update token last used time: Repository not initialized"));
        return false;
    }

    QMap<QString, QVariant> params;
    params["token_id"] = token;
    params["last_used_at"] = QDateTime::currentDateTimeUtc().toUTC();
    params["updated_at"] = QDateTime::currentDateTimeUtc().toUTC();

    QString query = buildUpdateLastUsedQuery();

    // Log query before execution
    logQueryWithValues(query, params);

    bool success = executeModificationQuery(query, params);

    if (success) {
        LOG_DEBUG(QString("Updated last used time for token: %1").arg(token));
    } else {
        LOG_WARNING(QString("Failed to update last used time for token: %1 - %2")
                   .arg(token, lastError()));
    }

    return success;
}

bool TokenRepository::tokenExists(const QString& token)
{
    if (!ensureInitialized()) {
        LOG_ERROR(QString("Cannot check token existence: Repository not initialized"));
        return false;
    }

    QMap<QString, QVariant> params;
    params["id"] = token;

    // We're actually checking by token_id which is a string, not the primary key UUID
    QString query = "SELECT EXISTS(SELECT 1 FROM auth_tokens WHERE token_id = :id)";

    // Log query before execution
    logQueryWithValues(query, params);

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> TokenModel* {
            TokenModel* existsResult = new TokenModel();
            // Store the exists flag in a property that can be retrieved
            QJsonObject existsData;
            existsData["exists"] = query.value(0).toBool();
            existsResult->setProperty("existsData", existsData);
            return existsResult;
        }
    );

    bool exists = false;
    if (result) {
        // Extract exists flag from the model
        QJsonObject existsData = (*result)->property("existsData").toJsonObject();
        exists = existsData["exists"].toBool();
        delete *result;
    }

    LOG_DEBUG(QString("Token existence check: %1 - %2").arg(token).arg(exists ? "exists" : "not found"));
    return exists;
}

QList<QSharedPointer<TokenModel>> TokenRepository::getTokensByUserId(const QUuid &userId)
{
    LOG_DEBUG(QString("Getting tokens for user ID: %1").arg(userId.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get tokens by user ID: Repository not initialized");
        return QList<QSharedPointer<TokenModel>>();
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);

    QString query = "SELECT * FROM auth_tokens WHERE user_id = :user_id ORDER BY created_at DESC";

    auto tokens = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> TokenModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<TokenModel>> result;
    for (auto token : tokens) {
        result.append(QSharedPointer<TokenModel>(token));
    }

    LOG_INFO(QString("Retrieved %1 tokens for user ID: %2").arg(tokens.count()).arg(userId.toString()));
    return result;
}

QSharedPointer<TokenModel> TokenRepository::getByTokenId(const QString &tokenId)
{
    LOG_DEBUG(QString("Getting token by ID: %1").arg(tokenId));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get token: Repository not initialized");
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["token_id"] = tokenId;

    QString query = "SELECT * FROM auth_tokens WHERE token_id = :token_id";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> TokenModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("Token found: %1").arg(tokenId));
        return QSharedPointer<TokenModel>(*result);
    } else {
        LOG_WARNING(QString("Token not found: %1").arg(tokenId));
        return nullptr;
    }
}

