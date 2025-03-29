#ifndef TOKENMODEL_H
#define TOKENMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>

/**
 * @brief Model representing an authentication token
 *
 * This class represents authentication tokens used for authorizing users,
 * services, and API access. Tokens contain information about their owner,
 * their validity period, and their status.
 */
class TokenModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString tokenId READ tokenId WRITE setTokenId NOTIFY tokenIdChanged)
    Q_PROPERTY(QString tokenType READ tokenType WRITE setTokenType NOTIFY tokenTypeChanged)
    Q_PROPERTY(QUuid userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QJsonObject tokenData READ tokenData WRITE setTokenData NOTIFY tokenDataChanged)
    Q_PROPERTY(QDateTime expiresAt READ expiresAt WRITE setExpiresAt NOTIFY expiresAtChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)
    Q_PROPERTY(bool revoked READ isRevoked WRITE setRevoked NOTIFY revokedChanged)
    Q_PROPERTY(QString revocationReason READ revocationReason WRITE setRevocationReason NOTIFY revocationReasonChanged)
    Q_PROPERTY(QJsonObject deviceInfo READ deviceInfo WRITE setDeviceInfo NOTIFY deviceInfoChanged)
    Q_PROPERTY(QDateTime lastUsedAt READ lastUsedAt WRITE setLastUsedAt NOTIFY lastUsedAtChanged)

public:
    explicit TokenModel(QObject *parent = nullptr);

    /**
     * @brief Get token's UUID (primary key in database)
     * @return Token's UUID
     */
    QUuid id() const;

    /**
     * @brief Set token's UUID
     * @param id Token's UUID
     */
    void setId(const QUuid &id);

    /**
     * @brief Get token's string identifier
     * @return Token string value
     */
    QString tokenId() const;

    /**
     * @brief Set token's string identifier
     * @param tokenId Token string value
     */
    void setTokenId(const QString &tokenId);

    /**
     * @brief Get token type (user, service, api, refresh)
     * @return Token type
     */
    QString tokenType() const;

    /**
     * @brief Set token type
     * @param tokenType Token type
     */
    void setTokenType(const QString &tokenType);

    /**
     * @brief Get user ID associated with this token
     * @return User ID
     */
    QUuid userId() const;

    /**
     * @brief Set user ID associated with this token
     * @param userId User ID
     */
    void setUserId(const QUuid &userId);

    /**
     * @brief Get token data containing additional information
     * @return Token data as JSON object
     */
    QJsonObject tokenData() const;

    /**
     * @brief Set token data
     * @param tokenData Token data as JSON object
     */
    void setTokenData(const QJsonObject &tokenData);

    /**
     * @brief Get token expiration datetime
     * @return Expiration datetime
     */
    QDateTime expiresAt() const;

    /**
     * @brief Set token expiration datetime
     * @param expiresAt Expiration datetime
     */
    void setExpiresAt(const QDateTime &expiresAt);

    /**
     * @brief Get token creation datetime
     * @return Creation datetime
     */
    QDateTime createdAt() const;

    /**
     * @brief Set token creation datetime
     * @param createdAt Creation datetime
     */
    void setCreatedAt(const QDateTime &createdAt);

    /**
     * @brief Get ID of user who created the token
     * @return Creator's user ID
     */
    QUuid createdBy() const;

    /**
     * @brief Set ID of user who created the token
     * @param createdBy Creator's user ID
     */
    void setCreatedBy(const QUuid &createdBy);

    /**
     * @brief Get last update datetime
     * @return Last update datetime
     */
    QDateTime updatedAt() const;

    /**
     * @brief Set last update datetime
     * @param updatedAt Last update datetime
     */
    void setUpdatedAt(const QDateTime &updatedAt);

    /**
     * @brief Get ID of user who last updated the token
     * @return Updater's user ID
     */
    QUuid updatedBy() const;

    /**
     * @brief Set ID of user who last updated the token
     * @param updatedBy Updater's user ID
     */
    void setUpdatedBy(const QUuid &updatedBy);

    /**
     * @brief Check if token is revoked
     * @return True if token is revoked, false otherwise
     */
    bool isRevoked() const;

    /**
     * @brief Set token revocation status
     * @param revoked True to revoke token, false otherwise
     */
    void setRevoked(bool revoked);

    /**
     * @brief Get reason for token revocation
     * @return Revocation reason
     */
    QString revocationReason() const;

    /**
     * @brief Set reason for token revocation
     * @param reason Revocation reason
     */
    void setRevocationReason(const QString &reason);

    /**
     * @brief Get device information for this token
     * @return Device information as JSON object
     */
    QJsonObject deviceInfo() const;

    /**
     * @brief Set device information for this token
     * @param deviceInfo Device information as JSON object
     */
    void setDeviceInfo(const QJsonObject &deviceInfo);

    /**
     * @brief Get datetime when token was last used
     * @return Last used datetime
     */
    QDateTime lastUsedAt() const;

    /**
     * @brief Set datetime when token was last used
     * @param lastUsedAt Last used datetime
     */
    void setLastUsedAt(const QDateTime &lastUsedAt);

    /**
     * @brief Check if token is expired
     * @return True if current time is after expiration time
     */
    bool isExpired() const;

    /**
     * @brief Check if token is valid (not expired and not revoked)
     * @return True if token is valid, false otherwise
     */
    bool isValid() const;

signals:
    void idChanged(const QUuid &id);
    void tokenIdChanged(const QString &tokenId);
    void tokenTypeChanged(const QString &tokenType);
    void userIdChanged(const QUuid &userId);
    void tokenDataChanged(const QJsonObject &tokenData);
    void expiresAtChanged(const QDateTime &expiresAt);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);
    void revokedChanged(bool revoked);
    void revocationReasonChanged(const QString &reason);
    void deviceInfoChanged(const QJsonObject &deviceInfo);
    void lastUsedAtChanged(const QDateTime &lastUsedAt);

private:
    QUuid m_id;
    QString m_tokenId;
    QString m_tokenType;
    QUuid m_userId;
    QJsonObject m_tokenData;
    QDateTime m_expiresAt;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;
    bool m_revoked = false;
    QString m_revocationReason;
    QJsonObject m_deviceInfo;
    QDateTime m_lastUsedAt;
};

#endif // TOKENMODEL_H