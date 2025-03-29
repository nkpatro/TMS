#ifndef AUTHFRAMEWORK_H
#define AUTHFRAMEWORK_H

#include <QObject>
#include <QHttpServerRequest>
#include <QJsonObject>
#include <QUuid>
#include <QMap>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMutex>

class AuthController;
class UserRepository;
class UserModel;
class RoleRepository;
class TokenRepository;

class AuthFramework : public QObject {
    Q_OBJECT

public:
    // Authorization levels
    enum AuthLevel {
        None,       // No authentication required
        Basic,      // Basic authentication (valid token)
        User,       // User level authentication
        Admin,      // Admin level authorization 
        SuperAdmin  // Super admin authorization
    };

    // Token types
    enum TokenType {
        UserToken,      // Regular user token
        ServiceToken,   // Service-to-service token
        ApiKey,         // Long-lived API key
        RefreshToken    // Token used for refreshing user tokens
    };

    static AuthFramework& instance();

    // Token extraction methods
    QString extractToken(const QHttpServerRequest& request);
    QString extractServiceToken(const QHttpServerRequest& request);
    QString extractApiKey(const QHttpServerRequest& request);

    // Token validation methods
    bool validateServiceToken(const QString& token, QJsonObject& tokenData);
    bool validateApiKey(const QString& key, QJsonObject& apiKeyData);

    // Token generation methods
    TokenRepository* getTokenRepository() const { return m_tokenRepository; }\
        /**
     * @brief Set the token repository for database operations
     * @param tokenRepository Pointer to initialized token repository
     */
    void setTokenRepository(TokenRepository* tokenRepository);

    /**
     * @brief Initialize token storage from database
     * @return True if successful, false otherwise
     */
    bool initializeTokenStorage();

    /**
     * @brief Validate a token and retrieve user data
     * @param token Token string to validate
     * @param userData Output parameter for user data
     * @return True if token is valid, false otherwise
     */
    bool validateToken(const QString& token, QJsonObject& userData);

    /**
     * @brief Generate a new token for a user
     * @param userData User data to include in token
     * @param expiryHours Token validity period in hours
     * @return Generated token string
     */
    QString generateToken(const QJsonObject& userData, int expiryHours = 24);

    /**
     * @brief Generate a refresh token for a user
     * @param userData User data to include in token
     * @param expiryDays Token validity period in days
     * @return Generated refresh token string
     */
    QString generateRefreshToken(const QJsonObject& userData, int expiryDays = 30);

    /**
     * @brief Refresh a user token using a refresh token
     * @param refreshToken Refresh token string
     * @param newToken Output parameter for new token
     * @param userData Output parameter for user data
     * @return True if refresh was successful
     */
    bool refreshUserToken(const QString& refreshToken, QString& newToken, QJsonObject& userData);

    /**
     * @brief Revoke a token
     * @param token Token to revoke
     * @return True if token was revoked
     */
    bool removeToken(const QString& token);

    /**
     * @brief Remove expired tokens from memory and database
     */
    void purgeExpiredTokens();

    /**
     * @brief Check if a token is expired
     * @param tokenData Token data containing expiry information
     * @return True if token is expired
     */
    bool isTokenExpired(const QJsonObject& tokenData) const;

    QString generateServiceToken(
        const QString& serviceId,
        const QString& username,
        const QString& computerName,
        const QString& machineId,
        int expiryDays = 7);
    QString generateApiKey(
        const QString& serviceId,
        const QString& description,
        const QUuid& createdBy);

    // Authorization methods
    bool authorizeRequest(const QHttpServerRequest& request, QJsonObject& userData, bool strictMode = false);
    bool isReportEndpoint(const QString& path) const;
    bool hasRole(const QJsonObject& userData, const QString& role);
    bool hasPermission(const QJsonObject& userData, const QString& permission);
    
    // Role-based authorization
    bool requiresRole(const QHttpServerRequest& request, const QString& role, QJsonObject& userData);
    bool requiresPermission(const QHttpServerRequest& request, const QString& permission, QJsonObject& userData);
    bool requiresAuthLevel(const QHttpServerRequest& request, AuthLevel level, QJsonObject& userData);

    // User validation methods
    QSharedPointer<UserModel> validateAndGetUserForTracking(const QString& username);

    // Configuration
    void setAuthController(AuthController* authController);
    void setUserRepository(UserRepository* userRepository);
    void setRoleRepository(RoleRepository* roleRepository);
    void setAutoCreateUsers(bool autoCreate);
    void setEmailDomain(const QString& domain);
    void setTokenExpiry(TokenType tokenType, int hours);

    // Utility methods
    QString createDefaultEmail(const QString& username) const;
    QString hashPassword(const QString& password) const;
    
    // Audit logging
    void logAuthEvent(const QString& eventType, const QJsonObject& eventData);

private:
    // Private constructor for singleton
    AuthFramework(QObject* parent = nullptr);
    ~AuthFramework();

    // Prevent copy and assignment
    AuthFramework(const AuthFramework&) = delete;
    AuthFramework& operator=(const AuthFramework&) = delete;

    // Member variables
    AuthController* m_authController = nullptr;
    UserRepository* m_userRepository = nullptr;
    RoleRepository* m_roleRepository = nullptr;
    TokenRepository* m_tokenRepository = nullptr;
    QMap<QString, QJsonObject> m_tokenToUserData;
    QMap<QString, QJsonObject> m_serviceTokens;
    QMap<QString, QJsonObject> m_apiKeys;
    QMap<QString, QJsonObject> m_refreshTokens;
    QMap<TokenType, int> m_tokenExpiryHours;
    QString m_emailDomain = "redefine.co";
    bool m_autoCreateUsers = true;
    QMutex m_tokenMutex;
    QMutex m_apiKeyMutex;

    // Helper methods
    QJsonObject userToJson(UserModel* user) const;
    QString generateHash(const QString& input) const;
    QByteArray generateHash(const QByteArray& input) const;
};

#endif // AUTHFRAMEWORK_H