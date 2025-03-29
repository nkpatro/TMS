// Enhanced AuthFramework.cpp
#include "AuthFramework.h"
#include "Controllers/AuthController.h"
#include "Repositories/UserRepository.h"
#include "Repositories/RoleRepository.h"
#include "Repositories/TokenRepository.h"
#include "Models/UserModel.h"
#include "Models/RoleModel.h"
#include "Utils/SystemInfo.h"
#include "logger/logger.h"

#include <QUuid>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QJsonDocument>
#include <QMutexLocker>

AuthFramework& AuthFramework::instance() {
    static AuthFramework instance;
    return instance;
}

AuthFramework::AuthFramework(QObject* parent) : QObject(parent) {
    LOG_INFO("AuthFramework created");
    
    // Initialize token expiry times
    m_tokenExpiryHours[UserToken] = 24;       // 1 day
    m_tokenExpiryHours[ServiceToken] = 168;   // 7 days
    m_tokenExpiryHours[ApiKey] = 8760;        // 1 year
    m_tokenExpiryHours[RefreshToken] = 720;   // 30 days
}

AuthFramework::~AuthFramework() {
    LOG_INFO("AuthFramework destroyed");
}

void AuthFramework::setAuthController(AuthController* authController) {
    m_authController = authController;
}

void AuthFramework::setUserRepository(UserRepository* userRepository) {
    m_userRepository = userRepository;
}

void AuthFramework::setRoleRepository(RoleRepository* roleRepository) {
    m_roleRepository = roleRepository;
}

void AuthFramework::setAutoCreateUsers(bool autoCreate) {
    m_autoCreateUsers = autoCreate;
}

void AuthFramework::setEmailDomain(const QString& domain) {
    m_emailDomain = domain;
}

void AuthFramework::setTokenExpiry(TokenType tokenType, int hours) {
    m_tokenExpiryHours[tokenType] = hours;
}

QString AuthFramework::extractToken(const QHttpServerRequest& request) {
    QByteArray authHeader = request.value("Authorization");
    if (authHeader.isEmpty() || !authHeader.startsWith("Bearer ")) {
        LOG_DEBUG("No bearer token found in request");
        return QString();
    }

    QString token = QString::fromUtf8(authHeader.mid(7));
    LOG_DEBUG(QString("Token extracted from request: %1").arg(token));
    return token;
}

QString AuthFramework::extractServiceToken(const QHttpServerRequest& request) {
    QByteArray authHeader = request.value("Authorization");
    if (authHeader.isEmpty() || !authHeader.startsWith("ServiceToken ")) {
        LOG_DEBUG("No service token found in request");
        return QString();
    }

    QString token = QString::fromUtf8(authHeader.mid(13));
    LOG_DEBUG(QString("Service token extracted from request: %1").arg(token));
    return token;
}

QString AuthFramework::extractApiKey(const QHttpServerRequest& request) {
    QByteArray authHeader = request.value("X-API-Key");
    if (authHeader.isEmpty()) {
        LOG_DEBUG("No API key found in request");
        return QString();
    }

    QString key = QString::fromUtf8(authHeader);
    LOG_DEBUG(QString("API key extracted from request: %1").arg(key));
    return key;
}

bool AuthFramework::validateToken(const QString& token, QJsonObject& userData) {
    QMutexLocker locker(&m_tokenMutex);
    
    // First check in-memory cache
    if (m_tokenToUserData.contains(token)) {
        userData = m_tokenToUserData[token];

        // Check if token is expired
        if (isTokenExpired(userData)) {
            LOG_WARNING(QString("Token validation failed: Token expired - %1").arg(token));
            m_tokenToUserData.remove(token);
            return false;
        }

        LOG_DEBUG(QString("Token validated successfully from memory: %1 (%2)")
                .arg(userData["name"].toString(), userData["id"].toString()));
        return true;
    }

    // If not in memory, try the database using the repository
    if (m_tokenRepository && m_tokenRepository->isInitialized()) {
        bool valid = m_tokenRepository->validateToken(token, userData);

        if (valid) {
            // Add to in-memory cache
            m_tokenToUserData[token] = userData;
            LOG_DEBUG(QString("Token validated from database and added to memory: %1")
                     .arg(token));

            // Update last used time in database
            m_tokenRepository->updateTokenLastUsed(token);

            return true;
        }
    }

    LOG_WARNING(QString("Token validation failed: Token not found - %1").arg(token));
    return false;
}

bool AuthFramework::validateServiceToken(const QString& token, QJsonObject& tokenData) {
    QMutexLocker locker(&m_tokenMutex);
    
    if (!m_serviceTokens.contains(token)) {
        LOG_WARNING(QString("Service token not found: %1").arg(token));
        return false;
    }

    tokenData = m_serviceTokens[token];

    // Check if token is expired
    if (isTokenExpired(tokenData)) {
        LOG_WARNING(QString("Service token expired: %1").arg(token));
        m_serviceTokens.remove(token);
        return false;
    }

    LOG_DEBUG(QString("Service token validated for: %1 on %2")
            .arg(tokenData["service_id"].toString(), tokenData["computer_name"].toString()));

    return true;
}

bool AuthFramework::validateApiKey(const QString& key, QJsonObject& apiKeyData) {
    QMutexLocker locker(&m_apiKeyMutex);
    
    if (!m_apiKeys.contains(key)) {
        LOG_WARNING(QString("API key not found: %1").arg(key));
        return false;
    }

    apiKeyData = m_apiKeys[key];

    // Check if API key is expired
    if (isTokenExpired(apiKeyData)) {
        LOG_WARNING(QString("API key expired: %1").arg(key));
        m_apiKeys.remove(key);
        return false;
    }

    LOG_DEBUG(QString("API key validated for: %1 - %2")
            .arg(apiKeyData["service_id"].toString(), apiKeyData["description"].toString()));

    return true;
}

bool AuthFramework::authorizeRequest(const QHttpServerRequest& request, QJsonObject& userData, bool strictMode) {
    LOG_DEBUG("Checking request authorization");

    // First try Bearer token authentication
    QString token = extractToken(request);
    if (!token.isEmpty()) {
        // Validate the token
        bool authorized = validateToken(token, userData);
        if (authorized) {
            LOG_DEBUG(QString("Request authorized for user: %1").arg(userData["name"].toString()));
            return true;
        } else {
            LOG_WARNING("Invalid token, authorization failed");

            // In strict mode, reject invalid tokens
            if (strictMode) {
                LOG_WARNING("Strict authentication required and token is invalid");
                return false;
            }
        }
    }

    // Try API key authentication
    QString apiKey = extractApiKey(request);
    if (!apiKey.isEmpty()) {
        QJsonObject apiKeyData;
        bool validApiKey = validateApiKey(apiKey, apiKeyData);
        if (validApiKey) {
            // API keys are associated with a service, not a user
            userData = apiKeyData;
            LOG_DEBUG(QString("Request authorized with API key for service: %1").arg(apiKeyData["service_id"].toString()));
            return true;
        } else {
            LOG_WARNING("Invalid API key, authorization failed");
            if (strictMode) {
                LOG_WARNING("Strict authentication required and API key is invalid");
                return false;
            }
        }
    }

    // Try service token authentication
    QString serviceToken = extractServiceToken(request);
    if (!serviceToken.isEmpty()) {
        QJsonObject serviceTokenData;
        bool validServiceToken = validateServiceToken(serviceToken, serviceTokenData);
        if (validServiceToken) {
            // Service tokens are associated with a service and username
            userData = serviceTokenData;
            LOG_DEBUG(QString("Request authorized with service token for: %1").arg(serviceTokenData["username"].toString()));
            return true;
        } else {
            LOG_WARNING("Invalid service token, authorization failed");
            if (strictMode) {
                LOG_WARNING("Strict authentication required and service token is invalid");
                return false;
            }
        }
    }

    // If no token provided or invalid token in non-strict mode
    LOG_DEBUG("No valid authentication found in request");

    // In strict mode, require authentication
    if (strictMode) {
        LOG_WARNING("Strict authentication required but no valid authentication provided");
        return false;
    }

    // For non-strict mode, we'll require the request to provide user identification info
    LOG_INFO("Authentication not required for this endpoint. User will need to be identified in the request payload.");
    return true;
}

bool AuthFramework::isReportEndpoint(const QString& path) const {
    return path.contains("/reports/") ||
           path.contains("/statistics/") ||
           path.contains("/analytics/") ||
           path.contains("/stats") ||
           path.contains("/summary") ||
           path.contains("/chart") ||
           path.contains("/metrics") ||
           path.contains("/timeseries");
}

bool AuthFramework::hasRole(const QJsonObject& userData, const QString& role) {
    if (!userData.contains("roles")) {
        return false;
    }

    QJsonArray roles = userData["roles"].toArray();
    for (const auto& r : roles) {
        if (r.toString() == role) {
            return true;
        }
    }

    return false;
}

bool AuthFramework::hasPermission(const QJsonObject& userData, const QString& permission) {
    if (!userData.contains("permissions")) {
        return false;
    }

    QJsonArray permissions = userData["permissions"].toArray();
    for (const auto& p : permissions) {
        if (p.toString() == permission) {
            return true;
        }
    }

    return false;
}

bool AuthFramework::requiresRole(const QHttpServerRequest& request, const QString& role, QJsonObject& userData) {
    if (!authorizeRequest(request, userData, true)) {
        return false;
    }

    return hasRole(userData, role);
}

bool AuthFramework::requiresPermission(const QHttpServerRequest& request, const QString& permission, QJsonObject& userData) {
    if (!authorizeRequest(request, userData, true)) {
        return false;
    }

    return hasPermission(userData, permission);
}

bool AuthFramework::requiresAuthLevel(const QHttpServerRequest& request, AuthLevel level, QJsonObject& userData) {
    // Basic auth check for anything above None
    if (level > None && !authorizeRequest(request, userData, true)) {
        return false;
    }

    // For None, we're always good
    if (level == None) {
        return true;
    }

    // For User level, just being authenticated is enough
    if (level == User) {
        return true;
    }

    // For Admin level, check for admin role
    if (level == Admin && !hasRole(userData, "admin")) {
        return false;
    }

    // For SuperAdmin level, check for superadmin role
    if (level == SuperAdmin && !hasRole(userData, "superadmin")) {
        return false;
    }

    return true;
}

QString AuthFramework::generateToken(const QJsonObject& userData, int expiryHours) {
    LOG_DEBUG(QString("Generating token for user: %1").arg(userData["name"].toString()));

    QDateTime expiryTime = QDateTime::currentDateTimeUtc().addSecs(expiryHours * 3600);
    
    // Create token data with expiry
    QJsonObject tokenData = userData;
    tokenData["expires_at"] = expiryTime.toString(Qt::ISODate);
    tokenData["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    tokenData["token_id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Generate token
    QByteArray tokenBase = generateHash(QJsonDocument(tokenData).toJson());
    QString token = tokenBase.toHex();

    // Store token data in memory map
    QMutexLocker locker(&m_tokenMutex);
    m_tokenToUserData[token] = tokenData;

    // Store in database if repository is available
    if (m_tokenRepository && m_tokenRepository->isInitialized()) {
        QUuid userId(userData["id"].toString());

        // Get the creator's ID if available
        QUuid createdBy;
        if (userData.contains("current_user_id")) {
            createdBy = QUuid(userData["current_user_id"].toString());
        }

        // Use the improved saveToken method
        m_tokenRepository->saveToken(token, "user", userId, tokenData, expiryTime, createdBy);
    }

    LOG_INFO(QString("Token generated for user: %1 (expires: %2)")
            .arg(userData["name"].toString(), expiryTime.toString(Qt::ISODate)));

    return token;
}

QString AuthFramework::generateServiceToken(
    const QString& serviceId,
    const QString& username,
    const QString& computerName,
    const QString& machineId,
    int expiryDays)
{
    LOG_DEBUG(QString("Generating service token for: %1, %2, %3").arg(serviceId, username, computerName));

    // Get expiry time in hours for service tokens
    int expiryHours = m_tokenExpiryHours[ServiceToken];
    if (expiryDays > 0) {
        expiryHours = expiryDays * 24;
    }
    
    QDateTime expiryTime = QDateTime::currentDateTimeUtc().addSecs(expiryHours * 3600);

    // Create token data
    QJsonObject tokenData;
    tokenData["service_id"] = serviceId;
    tokenData["username"] = username;
    tokenData["computer_name"] = computerName;
    tokenData["machine_id"] = machineId;
    tokenData["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    tokenData["expires_at"] = expiryTime.toString(Qt::ISODate);
    tokenData["token_id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Generate token
    QByteArray tokenBase = generateHash(QJsonDocument(tokenData).toJson());
    QString token = tokenBase.toHex();

    // Store token data
    QMutexLocker locker(&m_tokenMutex);
    m_serviceTokens[token] = tokenData;

    LOG_INFO(QString("Service token generated for: %1 on %2 (user: %3)")
            .arg(serviceId, computerName, username));

    return token;
}

QString AuthFramework::generateApiKey(
    const QString& serviceId,
    const QString& description,
    const QUuid& createdBy)
{
    LOG_DEBUG(QString("Generating API key for service: %1").arg(serviceId));

    // Get expiry time in hours for API keys (default: 1 year)
    int expiryHours = m_tokenExpiryHours[ApiKey];
    QDateTime expiryTime = QDateTime::currentDateTimeUtc().addSecs(expiryHours * 3600);

    // Create API key data
    QJsonObject keyData;
    keyData["service_id"] = serviceId;
    keyData["description"] = description;
    keyData["created_by"] = createdBy.toString(QUuid::WithoutBraces);
    keyData["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    keyData["expires_at"] = expiryTime.toString(Qt::ISODate);
    keyData["key_id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Generate a key with special prefix to distinguish it from tokens
    QByteArray randomData = generateHash(QJsonDocument(keyData).toJson());
    QString key = "apk_" + randomData.toHex().left(32);

    // Store API key data
    QMutexLocker locker(&m_apiKeyMutex);
    m_apiKeys[key] = keyData;

    LOG_INFO(QString("API key generated for service: %1 (expires: %2)")
            .arg(serviceId, expiryTime.toString(Qt::ISODate)));

    return key;
}

QString AuthFramework::generateRefreshToken(const QJsonObject& userData, int expiryDays) {
    LOG_DEBUG(QString("Generating refresh token for user: %1").arg(userData["name"].toString()));

    // Get expiry time in hours for refresh tokens (default: 30 days)
    int expiryHours = m_tokenExpiryHours[RefreshToken];
    if (expiryDays > 0) {
        expiryHours = expiryDays * 24;
    }
    
    QDateTime expiryTime = QDateTime::currentDateTimeUtc().addSecs(expiryHours * 3600);

    // Create token data
    QJsonObject tokenData = userData;
    tokenData["expires_at"] = expiryTime.toString(Qt::ISODate);
    tokenData["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    tokenData["token_id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    tokenData["is_refresh_token"] = true;

    // Generate token with special prefix to distinguish it as a refresh token
    QByteArray tokenBase = generateHash(QJsonDocument(tokenData).toJson());
    QString token = "rt_" + tokenBase.toHex();

    // Store token data
    QMutexLocker locker(&m_tokenMutex);
    m_refreshTokens[token] = tokenData;

    LOG_INFO(QString("Refresh token generated for user: %1 (expires: %2)")
            .arg(userData["name"].toString(), expiryTime.toString(Qt::ISODate)));

    return token;
}

bool AuthFramework::refreshUserToken(const QString& refreshToken, QString& newToken, QJsonObject& userData) {
    QMutexLocker locker(&m_tokenMutex);
    
    if (!m_refreshTokens.contains(refreshToken)) {
        LOG_WARNING(QString("Refresh token not found: %1").arg(refreshToken));
        return false;
    }

    userData = m_refreshTokens[refreshToken];
    
    // Check if token is expired
    if (isTokenExpired(userData)) {
        LOG_WARNING(QString("Refresh token expired: %1").arg(refreshToken));
        m_refreshTokens.remove(refreshToken);
        return false;
    }

    // Remove the refresh-token specific fields
    userData.remove("is_refresh_token");
    
    // Track the user for audit purposes
    QUuid userId;
    if (userData.contains("id")) {
        userId = QUuid(userData["id"].toString());
        // Add this for audit in token creation
        userData["current_user_id"] = userId.toString(QUuid::WithoutBraces);
    }

    // Generate a new token
    newToken = generateToken(userData);

    // Revoke the old refresh token in database
    if (m_tokenRepository && m_tokenRepository->isInitialized()) {
        m_tokenRepository->revokeToken(refreshToken, "Used for token refresh");
    }

    // Remove the old refresh token from memory
    m_refreshTokens.remove(refreshToken);
    
    // Generate a new refresh token
    generateRefreshToken(userData);

    LOG_INFO(QString("Token refreshed for user: %1").arg(userData["name"].toString()));
    return true;
}

QSharedPointer<UserModel> AuthFramework::validateAndGetUserForTracking(const QString& username) {
    LOG_DEBUG(QString("Validating user exists for tracking: %1").arg(username));

    if (!m_userRepository) {
        LOG_ERROR("User repository not available");
        return nullptr;
    }

    // Check if user already exists in our database
    QString email = createDefaultEmail(username);
    auto user = m_userRepository->getByName(username);

    if (user) {
        LOG_DEBUG(QString("User found in database: %1").arg(username));
        return user;
    }

    // User not found, create a new one if auto-create is enabled
    if (m_autoCreateUsers) {
        LOG_INFO(QString("Creating user: %1").arg(username));

        UserModel* newUser = new UserModel();
        newUser->setName(username);
        newUser->setEmail(email);

        // Set a random password
        QString randomPassword = QUuid::createUuid().toString().remove("{").remove("}");
        newUser->setPassword(randomPassword);

        // Mark as active but not verified
        newUser->setActive(true);
        newUser->setVerified(false);

        // Set creation timestamps
        QDateTime now = QDateTime::currentDateTimeUtc();
        newUser->setCreatedAt(now);
        newUser->setUpdatedAt(now);

        // Save the user
        if (m_userRepository->save(newUser)) {
            LOG_INFO(QString("User created successfully: %1 <%2>").arg(username, email));
            return QSharedPointer<UserModel>(newUser);
        } else {
            LOG_ERROR(QString("Failed to create user: %1 <%2>").arg(username, email));
            delete newUser;
            return nullptr;
        }
    }

    LOG_WARNING(QString("User not found and auto-create disabled: %1").arg(username));
    return nullptr;
}

bool AuthFramework::removeToken(const QString& token) {
    LOG_DEBUG(QString("Removing token: %1").arg(token));
    
    QMutexLocker locker(&m_tokenMutex);
    bool removed = m_tokenToUserData.remove(token) > 0;

    // Revoke in database if repository is available
    if (m_tokenRepository && m_tokenRepository->isInitialized()) {
        QJsonObject userData;
        QString reason = "User logout";

        // Get user ID for audit trail if it's in the cache
        QUuid updatedBy;
        if (m_tokenToUserData.contains(token)) {
            userData = m_tokenToUserData[token];
            if (userData.contains("id")) {
                updatedBy = QUuid(userData["id"].toString());
            }
            reason = "Logout by user: " + userData["name"].toString();
        }

        // Use the revokeToken method to properly flag it in the database
        m_tokenRepository->revokeToken(token, reason);
    }

    return removed;
}

void AuthFramework::purgeExpiredTokens() {
    QMutexLocker tokenLocker(&m_tokenMutex);
    QMutexLocker apiKeyLocker(&m_apiKeyMutex);
    
    LOG_DEBUG("Purging expired tokens and API keys from memory");

    QDateTime now = QDateTime::currentDateTimeUtc();
    int purgedCount = 0;

    // Check user tokens
    QMutableMapIterator<QString, QJsonObject> tokenIt(m_tokenToUserData);
    while (tokenIt.hasNext()) {
        tokenIt.next();
        if (isTokenExpired(tokenIt.value())) {
            tokenIt.remove();
            purgedCount++;
        }
    }

    // Check service tokens
    QMutableMapIterator<QString, QJsonObject> serviceTokenIt(m_serviceTokens);
    while (serviceTokenIt.hasNext()) {
        serviceTokenIt.next();
        if (isTokenExpired(serviceTokenIt.value())) {
            serviceTokenIt.remove();
            purgedCount++;
        }
    }

    // Check refresh tokens
    QMutableMapIterator<QString, QJsonObject> refreshTokenIt(m_refreshTokens);
    while (refreshTokenIt.hasNext()) {
        refreshTokenIt.next();
        if (isTokenExpired(refreshTokenIt.value())) {
            refreshTokenIt.remove();
            purgedCount++;
        }
    }

    // Check API keys
    QMutableMapIterator<QString, QJsonObject> apiKeyIt(m_apiKeys);
    while (apiKeyIt.hasNext()) {
        apiKeyIt.next();
        if (isTokenExpired(apiKeyIt.value())) {
            apiKeyIt.remove();
            purgedCount++;
        }
    }

    LOG_INFO(QString("Purged %1 expired tokens and API keys from memory").arg(purgedCount));

    // If token repository is available, also purge from database
    if (m_tokenRepository && m_tokenRepository->isInitialized()) {
        int dbPurged = m_tokenRepository->purgeExpiredTokens();
        LOG_INFO(QString("Purged %1 expired tokens from database").arg(dbPurged));
    }
}

QString AuthFramework::createDefaultEmail(const QString& username) const {
    return username + "@" + m_emailDomain;
}

QString AuthFramework::hashPassword(const QString& password) const {
    return generateHash(password.toUtf8()).toHex();
}

void AuthFramework::logAuthEvent(const QString& eventType, const QJsonObject& eventData) {
    LOG_INFO(QString("Auth event: %1").arg(eventType));
    
    // This could be expanded to log to an audit table in the database
    // or to a specialized audit log file
    if (eventData.isEmpty()) {
        return;
    }
    
    // Log relevant details from the event data
    QStringList dataStrings;
    for (auto it = eventData.constBegin(); it != eventData.constEnd(); ++it) {
        QString key = it.key();
        // Don't log sensitive values
        if (key == "password" || key == "token" || key == "api_key") {
            continue;
        }
        dataStrings.append(QString("%1: %2").arg(key, it.value().toString()));
    }
    
    LOG_DEBUG(QString("Auth event details: %1").arg(dataStrings.join(", ")));
}

QJsonObject AuthFramework::userToJson(UserModel* user) const {
    if (!user) {
        return QJsonObject();
    }

    QJsonObject json;
    json["id"] = user->id().toString(QUuid::WithoutBraces);
    json["name"] = user->name();
    json["email"] = user->email();
    json["photo"] = user->photo();
    json["active"] = user->active();
    json["verified"] = user->verified();
    json["created_at"] = user->createdAt().toString(Qt::ISODate);
    json["updated_at"] = user->updatedAt().toString(Qt::ISODate);

    if (!user->statusId().isNull()) {
        json["status_id"] = user->statusId().toString(QUuid::WithoutBraces);
    }

    // Add role information if role repository is available
    if (m_roleRepository) {
        QJsonArray roles;
        
        // This would need to be implemented with appropriate queries to get user roles
        // For example:
        // QList<QSharedPointer<RoleModel>> userRoles = m_roleRepository->getRolesForUser(user->id());
        // for (const auto& role : userRoles) {
        //     roles.append(role->code());
        // }
        
        json["roles"] = roles;
    }

    return json;
}

bool AuthFramework::isTokenExpired(const QJsonObject& tokenData) const {
    if (!tokenData.contains("expires_at")) {
        return false;
    }
    
    QDateTime expiryTime = QDateTime::fromString(tokenData["expires_at"].toString(), Qt::ISODate);
    return QDateTime::currentDateTimeUtc() > expiryTime;
}

QString AuthFramework::generateHash(const QString& input) const {
    return generateHash(input.toUtf8());
}

QByteArray AuthFramework::generateHash(const QByteArray& input) const {
    return QCryptographicHash::hash(input, QCryptographicHash::Sha256);
}

void AuthFramework::setTokenRepository(TokenRepository* tokenRepository) {
    m_tokenRepository = tokenRepository;
}

bool AuthFramework::initializeTokenStorage() {
    LOG_INFO("Initializing token storage");

    if (!m_tokenRepository || !m_tokenRepository->isInitialized()) {
        LOG_WARNING("Token repository not available or not initialized");
        return false;
    }

    // Load active tokens from database
    QMap<QString, QJsonObject> storedTokens;
    if (m_tokenRepository->loadActiveTokens(storedTokens)) {
        // Add to in-memory maps
        QMutexLocker locker(&m_tokenMutex);

        // Process each token based on its type
        for (auto it = storedTokens.constBegin(); it != storedTokens.constEnd(); ++it) {
            const QString& token = it.key();
            const QJsonObject& tokenData = it.value();

            // Check token type
            if (tokenData.contains("is_refresh_token") && tokenData["is_refresh_token"].toBool()) {
                m_refreshTokens[token] = tokenData;
                LOG_DEBUG(QString("Loaded refresh token: %1").arg(token));
            }
            else if (token.startsWith("apk_")) {
                m_apiKeys[token] = tokenData;
                LOG_DEBUG(QString("Loaded API key: %1").arg(token));
            }
            else if (tokenData.contains("service_id")) {
                m_serviceTokens[token] = tokenData;
                LOG_DEBUG(QString("Loaded service token: %1").arg(token));
            }
            else {
                m_tokenToUserData[token] = tokenData;
                LOG_DEBUG(QString("Loaded user token: %1").arg(token));
            }
        }

        LOG_INFO(QString("Loaded %1 tokens from database").arg(storedTokens.size()));
        return true;
    } else {
        LOG_ERROR("Failed to load tokens from database");
        return false;
    }
}

