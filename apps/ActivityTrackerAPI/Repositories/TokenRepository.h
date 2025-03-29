#ifndef TOKENREPOSITORY_H
#define TOKENREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/TokenModel.h"
#include "dbservice/dbservice.hpp"
#include "logger/logger.h"
#include <QJsonObject>
#include <QDateTime>

/**
 * @brief Repository for managing auth tokens in the database
 *
 * This class extends the BaseRepository to provide token-specific
 * database operations, including token storage, validation, and revocation.
 * It integrates with the dbservice and logger libraries for database access
 * and logging functionality.
 */
class TokenRepository : public BaseRepository<TokenModel>
{
    Q_OBJECT
public:
    explicit TokenRepository(QObject *parent = nullptr);

    /**
     * @brief Save a new token or update an existing one
     * @param token Token string
     * @param tokenType Type of token (user, service, refresh, etc.)
     * @param userId User ID associated with the token
     * @param tokenData Additional token data
     * @param expiryTime Token expiration time
     * @param createdBy User who created the token (optional)
     * @return True if successfully saved
     */
    bool saveToken(const QString& token,
                  const QString& tokenType,
                  const QUuid& userId,
                  const QJsonObject& tokenData,
                  const QDateTime& expiryTime,
                  const QUuid& createdBy = QUuid());

    /**
     * @brief Validate a token and retrieve its data
     * @param token Token string to validate
     * @param tokenData Output parameter for token data
     * @return True if token is valid
     */
    bool validateToken(const QString& token, QJsonObject& tokenData);

    /**
     * @brief Revoke a specific token
     * @param token Token string to revoke
     * @param reason Reason for revocation
     * @return True if successfully revoked
     */
    bool revokeToken(const QString& token, const QString& reason = QString());

    /**
     * @brief Revoke all tokens for a specific user
     * @param userId User ID
     * @param reason Reason for revocation
     * @return True if successfully revoked
     */
    bool revokeAllUserTokens(const QUuid& userId, const QString& reason = QString());

    /**
     * @brief Load all active tokens into a map
     * @param tokenMap Map to store token ID to token data mapping
     * @return True if successfully loaded
     */
    bool loadActiveTokens(QMap<QString, QJsonObject>& tokenMap);

    /**
     * @brief Purge expired tokens from the database
     * @return Number of tokens purged
     */
    int purgeExpiredTokens();

    /**
     * @brief Update the last used timestamp for a token
     * @param token Token string
     * @return True if successfully updated
     */
    bool updateTokenLastUsed(const QString& token);

    /**
     * @brief Check if a token exists in the database
     * @param token Token string
     * @return True if token exists
     */
    bool tokenExists(const QString& token);

protected:
    // Required BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(TokenModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(TokenModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(TokenModel* model) override;
    TokenModel* createModelFromQuery(const QSqlQuery &query) override;
    bool validateModel(TokenModel* model, QStringList& errors) override;

    // Additional getters that override BaseRepository defaults
    QString getTableName() const override;
    QString getIdParamName() const override;

    // Additional query helpers
    QString buildGetByTokenQuery();
    QString buildGetActiveTokensQuery();
    QString buildRevokeTokenQuery();
    QString buildRevokeAllUserTokensQuery();
    QString buildUpdateLastUsedQuery();
};

#endif // TOKENREPOSITORY_H