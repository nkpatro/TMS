#ifndef AUTHFRAMEWORK_H
#define AUTHFRAMEWORK_H

#include <QObject>
#include <QHttpServerRequest>
#include <QJsonObject>
#include <QUuid>
#include <QMap>
#include <QDateTime>
#include <QCryptographicHash>

class AuthController;
class UserRepository;
class UserModel;
class RoleRepository;
class TokenRepository;

/**
 * @brief Authentication and authorization framework singleton
 *
 * This class provides centralized authentication and authorization services
 * including token management, user validation, and permission checking.
 * It integrates with database repositories for persistence.
 */
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

    /**
     * @brief Get singleton instance
     * @return Reference to the singleton instance
     */
    static AuthFramework& instance();

    /**
     * @brief Set the auth controller
     * @param authController Pointer to the auth controller
     */
    void setAuthController(AuthController* authController);

    /**
     * @brief Set the user repository for user operations
     * @param userRepository Pointer to user repository
     */
    void setUserRepository(UserRepository* userRepository);

    /**
     * @brief Set the role repository for role operations
     * @param roleRepository Pointer to role repository
     */
    void setRoleRepository(RoleRepository* roleRepository);

    /**
     * @brief Set the token repository for database operations
     * @param tokenRepository Pointer to initialized token repository
     */
    void setTokenRepository(TokenRepository* tokenRepository);

    /**
     * @brief Get the token repository
     * @return Pointer to the token repository
     */
    TokenRepository* getTokenRepository() const { return m_tokenRepository; }

    /**
     * @brief Purge expired tokens from the database
     */
    void purgeExpiredTokens();

    // Token extraction methods

    /**
     * @brief Extract bearer token from HTTP request
     * @param request HTTP request
     * @return Extracted token or empty string if not found
     */
    QString extractToken(const QHttpServerRequest& request);

    /**
     * @brief Extract service token from HTTP request
     * @param request HTTP request
     * @return Extracted service token or empty string if not found
     */
    QString extractServiceToken(const QHttpServerRequest& request);

    /**
     * @brief Extract API key from HTTP request
     * @param request HTTP request
     * @return Extracted API key or empty string if not found
     */
    QString extractApiKey(const QHttpServerRequest& request);

    // Token validation methods

    /**
     * @brief Validate a token and retrieve user data
     * @param token Token string to validate
     * @param userData Output parameter for user data
     * @return True if token is valid, false otherwise
     */
    bool validateToken(const QString& token, QJsonObject& userData);

    /**
     * @brief Validate a service token and retrieve its data
     * @param token Service token string to validate
     * @param tokenData Output parameter for token data
     * @return True if token is valid, false otherwise
     */
    bool validateServiceToken(const QString& token, QJsonObject& tokenData);

    /**
     * @brief Validate an API key and retrieve its data
     * @param key API key string to validate
     * @param apiKeyData Output parameter for API key data
     * @return True if API key is valid, false otherwise
     */
    bool validateApiKey(const QString& key, QJsonObject& apiKeyData);

    // Token generation methods

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
     * @brief Generate a service token
     * @param serviceId Service identifier
     * @param username Username associated with the token
     * @param computerName Computer name
     * @param machineId Machine identifier
     * @param expiryDays Token validity period in days
     * @return Generated service token string
     */
    QString generateServiceToken(
        const QString& serviceId,
        const QString& username,
        const QString& computerName,
        const QString& machineId,
        int expiryDays = 7);

    /**
     * @brief Generate an API key
     * @param serviceId Service identifier
     * @param description API key description
     * @param createdBy User ID who created the key
     * @return Generated API key string
     */
    QString generateApiKey(
        const QString& serviceId,
        const QString& description,
        const QUuid& createdBy);

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
     * @brief Check if a token is expired
     * @param tokenData Token data containing expiry information
     * @return True if token is expired
     */
    bool isTokenExpired(const QJsonObject& tokenData) const;

    // Authorization methods

    /**
     * @brief Authorize a request based on various auth methods
     * @param request HTTP request
     * @param userData Output parameter for user data
     * @param strictMode Whether to require valid authentication
     * @return True if authorized, false otherwise
     */
    bool authorizeRequest(const QHttpServerRequest& request, QJsonObject& userData, bool strictMode = false);

    /**
     * @brief Check if a path is a report endpoint
     * @param path URL path
     * @return True if it's a report endpoint
     */
    bool isReportEndpoint(const QString& path) const;

    /**
     * @brief Check if user has a specific role
     * @param userData User data containing roles
     * @param role Role to check
     * @return True if user has the role
     */
    bool hasRole(const QJsonObject& userData, const QString& role);

    /**
     * @brief Check if user has a specific permission
     * @param userData User data containing permissions
     * @param permission Permission to check
     * @return True if user has the permission
     */
    bool hasPermission(const QJsonObject& userData, const QString& permission);

    // Role-based authorization

    /**
     * @brief Check if request has required role
     * @param request HTTP request
     * @param role Required role
     * @param userData Output parameter for user data
     * @return True if authorized with required role
     */
    bool requiresRole(const QHttpServerRequest& request, const QString& role, QJsonObject& userData);

    /**
     * @brief Check if request has required permission
     * @param request HTTP request
     * @param permission Required permission
     * @param userData Output parameter for user data
     * @return True if authorized with required permission
     */
    bool requiresPermission(const QHttpServerRequest& request, const QString& permission, QJsonObject& userData);

    /**
     * @brief Check if request has required authorization level
     * @param request HTTP request
     * @param level Required authorization level
     * @param userData Output parameter for user data
     * @return True if authorized with required level
     */
    bool requiresAuthLevel(const QHttpServerRequest& request, AuthLevel level, QJsonObject& userData);

    // User validation methods

    /**
     * @brief Validate and get user for tracking
     * @param username Username to validate
     * @return Shared pointer to user model or nullptr
     */
    QSharedPointer<UserModel> validateAndGetUserForTracking(const QString& username);

    // Configuration

    /**
     * @brief Enable or disable auto-creation of users
     * @param autoCreate Whether to auto-create users
     */
    void setAutoCreateUsers(bool autoCreate);

    /**
     * @brief Set the email domain for default emails
     * @param domain Email domain
     */
    void setEmailDomain(const QString& domain);

    /**
     * @brief Set token expiry time
     * @param tokenType Type of token
     * @param hours Expiry time in hours
     */
    void setTokenExpiry(TokenType tokenType, int hours);

    // Utility methods

    /**
     * @brief Create default email for a username
     * @param username Username
     * @return Generated email address
     */
    QString createDefaultEmail(const QString& username) const;

    /**
     * @brief Hash a password
     * @param password Plain password
     * @return Hashed password
     */
    QString hashPassword(const QString& password) const;

    // Audit logging

    /**
     * @brief Log an authentication event
     * @param eventType Type of event
     * @param eventData Event data
     */
    void logAuthEvent(const QString& eventType, const QJsonObject& eventData);

private:
    // Private constructor for singleton
    AuthFramework(QObject* parent = nullptr);
    ~AuthFramework();

    // Prevent copy and assignment
    AuthFramework(const AuthFramework&) = delete;
    AuthFramework& operator=(const AuthFramework&) = delete;

    // Helper methods for token/hash generation
    QString generateHash(const QString& input) const;
    QByteArray generateHash(const QByteArray& input) const;
    QJsonObject userToJson(UserModel* user) const;

    // Member variables
    AuthController* m_authController = nullptr;
    UserRepository* m_userRepository = nullptr;
    RoleRepository* m_roleRepository = nullptr;
    TokenRepository* m_tokenRepository = nullptr;

    // Configuration
    QMap<TokenType, int> m_tokenExpiryHours;
    QString m_emailDomain = "redefine.co";
    bool m_autoCreateUsers = true;
};

#endif // AUTHFRAMEWORK_H