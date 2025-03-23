#include "AuthController.h"
#include "Services/ADVerificationService.h"
#include <QJsonDocument>
#include <QUuid>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>

#include "SystemInfo.h"
#include "logger/logger.h"
#include "httpserver/response.h"

AuthController::AuthController(UserRepository *userRepository, ADVerificationService *adService, QObject *parent)
    : ApiControllerBase(parent),
      m_repository(userRepository),
      m_adService(adService),
      m_autoCreateUsers(true),
      m_emailDomain("redefine.co")
{
    LOG_INFO("AuthController created");
}

void AuthController::setupRoutes(QHttpServer &server)
{
    LOG_INFO("Setting up auth routes");

    // Login route
    server.route("/api/auth/login", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleLogin(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Logout route
    server.route("/api/auth/logout", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleLogout(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get user profile
    server.route("/api/auth/profile", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleGetProfile(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Refresh token
    server.route("/api/auth/refresh", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleRefreshToken(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // New endpoint: Service token generation
    server.route("/api/auth/service-token", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = handleServiceToken(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    LOG_INFO("Auth routes set up successfully");
}

QHttpServerResponse AuthController::handleLogin(const QHttpServerRequest &request)
{
    LOG_INFO("Login request received");

    // Extract and validate JSON payload
    bool ok;
    QJsonObject json = extractJsonFromRequest(request, ok);
    if (!ok) {
        LOG_WARNING("Invalid JSON data in login request");
        return Http::Response::badRequest("Invalid JSON data");
    }

    // Validate required fields - either email or username must be present with password
    QStringList missingFields;
    bool hasEmailOrUsername = json.contains("email") || json.contains("username");
    bool hasPassword = json.contains("password") && !json["password"].toString().isEmpty();

    if (!hasEmailOrUsername || !hasPassword) {
        QStringList errors;
        if (!hasEmailOrUsername) {
            errors.append("Either email or username is required");
        }
        if (!hasPassword) {
            errors.append("Password is required");
        }
        LOG_WARNING("Login attempt with missing credentials");
        return Http::Response::validationError("Missing required fields", {
            {"missing_fields", errors.join(", ")}
        });
    }

    // Extract credentials
    QString email = json["email"].toString();
    QString password = json["password"].toString();
    QString username = json.contains("username") ? json["username"].toString() : "";

    // If email is provided but username is not, extract username from email
    if (!email.isEmpty() && username.isEmpty()) {
        username = email.split('@').first();
    }

    LOG_DEBUG(QString("Login attempt for username: %1").arg(username));

    try {
        // Verify with AD using password validation
        QJsonObject adUserInfo;
        bool adVerified = m_adService->verifyUserCredentials(username, password, adUserInfo);

        if (!adVerified) {
            LOG_WARNING(QString("Login failed: Invalid AD credentials for user %1").arg(username));
            return Http::Response::unauthorized("Invalid credentials", "INVALID_CREDENTIALS");
        }

        LOG_INFO(QString("AD authentication successful for user: %1").arg(username));

        // Find or create the user in our database
        auto user = findOrCreateUserFromADInfo(username, adUserInfo);
        if (!user) {
            LOG_ERROR(QString("Failed to find or create user: %1").arg(username));
            return Http::Response::internalError("User creation failed");
        }

        // Check if user is active
        if (!user->active()) {
            LOG_WARNING(QString("Login failed: Inactive account for user %1").arg(username));
            return Http::Response::forbidden("Account is inactive", "ACCOUNT_INACTIVE");
        }

        // Log successful authentication
        AuthFramework::instance().logAuthEvent("user_login", QJsonObject{
            {"username", username},
            {"ip_address", request.remoteAddress().toString()},
            {"user_id", user->id().toString(QUuid::WithoutBraces)},
            {"success", true}
        });

        // Process successful login
        return processSuccessfulLogin(user);
    }
    catch (const std::exception& e) {
        LOG_ERROR(QString("Exception during login: %1").arg(e.what()));

        // Log failed authentication
        AuthFramework::instance().logAuthEvent("user_login_error", QJsonObject{
            {"username", username},
            {"ip_address", request.remoteAddress().toString()},
            {"error", e.what()}
        });

        return Http::Response::internalError(
            "An error occurred during authentication",
            "AUTH_ERROR"
        );
    }
}

QHttpServerResponse AuthController::handleLogout(const QHttpServerRequest &request)
{
    LOG_INFO("Logout request received");

    // Extract and validate token
    QString token = AuthFramework::instance().extractToken(request);
    if (token.isEmpty()) {
        LOG_WARNING("Logout attempt without token");
        return Http::Response::badRequest("No token provided");
    }

    QJsonObject userData;
    bool tokenValid = AuthFramework::instance().validateToken(token, userData);

    if (tokenValid) {
        // Remove token
        bool removed = AuthFramework::instance().removeToken(token);

        // Log event
        AuthFramework::instance().logAuthEvent("user_logout", QJsonObject{
            {"username", userData["name"].toString()},
            {"user_id", userData["id"].toString()},
            {"ip_address", request.remoteAddress().toString()},
            {"success", removed}
        });

        QJsonObject response;
        response["success"] = removed;
        response["message"] = removed ? "Logged out successfully" : "Token removal failed";

        LOG_INFO(QString("User logged out successfully: %1 (%2)")
                 .arg(userData["name"].toString(), userData["id"].toString()));

        return Http::Response::json(response);
    }
    else {
        LOG_WARNING(QString("Logout failed: Invalid token"));
        return Http::Response::unauthorized("Invalid token", "INVALID_TOKEN");
    }
}

QHttpServerResponse AuthController::handleGetProfile(const QHttpServerRequest &request)
{
    LOG_INFO("Profile request received");

    QJsonObject userData;
    if (!isUserAuthorized(request, userData, true)) {
        LOG_WARNING("Unauthorized profile request");
        return Http::Response::unauthorized("Unauthorized");
    }

    QUuid userId = QUuid(userData["id"].toString());
    auto user = m_repository->getById(userId);

    if (!user) {
        LOG_WARNING(QString("Profile not found for user ID: %1").arg(userId.toString()));
        return Http::Response::notFound("User not found");
    }

    LOG_INFO(QString("Profile retrieved successfully for user: %1 (%2)")
             .arg(user->name(), user->id().toString()));

    return Http::Response::json(userToJson(user.data()));
}

QHttpServerResponse AuthController::handleRefreshToken(const QHttpServerRequest &request)
{
    LOG_INFO("Token refresh request received");

    // Extract the refresh token from the request
    bool ok;
    QJsonObject json = extractJsonFromRequest(request, ok);

    if (!ok || !json.contains("refresh_token") || json["refresh_token"].toString().isEmpty()) {
        LOG_WARNING("Invalid or missing refresh token in request");
        return Http::Response::badRequest("Invalid or missing refresh token");
    }

    QString refreshToken = json["refresh_token"].toString();

    // Validate refresh token and generate new tokens
    QString newAccessToken;
    QJsonObject userData;

    if (AuthFramework::instance().refreshUserToken(refreshToken, newAccessToken, userData)) {
        // Create response with new tokens
        QJsonObject response;
        response["access_token"] = newAccessToken;
        response["token_type"] = "Bearer";
        response["expires_in"] = 3600; // 1 hour
        response["user"] = userData;

        LOG_INFO(QString("Token refreshed successfully for user: %1")
                 .arg(userData["name"].toString()));

        return Http::Response::json(response);
    }
    else {
        LOG_WARNING("Invalid or expired refresh token");
        return Http::Response::unauthorized("Invalid or expired refresh token", "INVALID_REFRESH_TOKEN");
    }
}

QHttpServerResponse AuthController::handleServiceToken(const QHttpServerRequest &request)
{
    LOG_INFO("Service token request received");

    // Parse request body
    bool ok;
    QJsonObject json = extractJsonFromRequest(request, ok);
    if (!ok) {
        LOG_WARNING("Invalid JSON data in service token request");
        return Http::Response::badRequest("Invalid JSON data");
    }

    // Validate required fields
    QStringList requiredFields = {"username", "service_id"};
    QStringList missingFields;
    if (!validateRequiredFields(json, requiredFields, missingFields)) {
        LOG_WARNING("Service token request missing required fields");
        return Http::Response::validationError("Missing required fields", {
            {"missing_fields", missingFields.join(", ")}
        });
    }

    // Extract fields
    QString username = json["username"].toString();
    QString serviceId = json["service_id"].toString();
    QString computerName = json.contains("computer_name") ? json["computer_name"].toString() :
                         SystemInfo::getMachineHostName();
    QString machineId = json.contains("machine_id") ? json["machine_id"].toString() : "";

    // Allow empty machine_id but generate one if possible
    if (machineId.isEmpty()) {
        LOG_WARNING("Service token request has empty machine_id");

        // Generate a consistent machine ID if possible
        if (!computerName.isEmpty()) {
            machineId = computerName + "-" + SystemInfo::getMacAddress();
            LOG_INFO(QString("Generated machine ID: %1").arg(machineId));
        }
    }

    LOG_INFO(QString("Service token requested: %1 on %2 (service: %3)")
            .arg(username, computerName, serviceId));

    try {
        // Validate or create the user
        auto user = validateAndGetUserForTracking(username);
        if (!user) {
            LOG_ERROR(QString("User validation failed for service token: %1").arg(username));
            return Http::Response::unprocessableEntity("User validation failed");
        }

        // Generate a service token
        QString token = AuthFramework::instance().generateServiceToken(
            serviceId, username, computerName, machineId);

        // Create response
        QJsonObject response;
        response["token"] = token;
        response["user"] = userToJson(user.data());
        response["service_id"] = serviceId;
        response["expires_at"] = QDateTime::currentDateTimeUtc().addDays(7).toString(Qt::ISODate);

        LOG_INFO(QString("Service token generated for user: %1 on machine: %2")
                .arg(username, computerName));

        return Http::Response::json(response);
    }
    catch (const std::exception& e) {
        LOG_ERROR(QString("Exception generating service token: %1").arg(e.what()));
        return Http::Response::internalError(
            QString("Failed to generate service token: %1").arg(e.what())
        );
    }
}

bool AuthController::isUserAuthorized(const QHttpServerRequest &request, QJsonObject &userData, bool strictMode)
{
    // First use the base class implementation which leverages AuthFramework
    if (ApiControllerBase::isUserAuthorized(request, userData, strictMode)) {
        return true;
    }

    // If strict mode and the base class failed authorization, we fail too
    if (strictMode) {
        return false;
    }

    // For non-strict mode, we'll check if we can extract a username from the request
    // and authenticate that way (useful for service-to-service comm)
    bool ok;
    QJsonObject requestData = extractJsonFromRequest(request, ok);

    if (ok && requestData.contains("username") && !requestData["username"].toString().isEmpty()) {
        QString username = requestData["username"].toString();
        auto user = validateAndGetUserForTracking(username);

        if (user) {
            userData["id"] = user->id().toString(QUuid::WithoutBraces);
            userData["name"] = user->name();
            userData["email"] = user->email();
            LOG_INFO(QString("User authorized via request body username: %1").arg(username));
            return true;
        }
    }

    return false;
}

QString AuthController::extractServiceToken(const QHttpServerRequest &request)
{
    return AuthFramework::instance().extractServiceToken(request);
}

bool AuthController::validateServiceToken(const QString &token, QJsonObject &tokenData)
{
    return AuthFramework::instance().validateServiceToken(token, tokenData);
}

QSharedPointer<UserModel> AuthController::validateAndGetUserForTracking(const QString &username)
{
    return AuthFramework::instance().validateAndGetUserForTracking(username);
}

QSharedPointer<UserModel> AuthController::findOrCreateUserByUsername(const QString &username)
{
    LOG_DEBUG(QString("Finding or creating user for username: %1").arg(username));

    // Create email from username using default domain
    QString email = createDefaultEmail(username);

    // Try to find user by email
    auto user = m_repository->getByEmail(email);
    if (user) {
        LOG_INFO(QString("Found existing user for username: %1").arg(username));
        return user;
    }

    // Create new user if not found
    return createNewUser(username, username, email);
}

QSharedPointer<UserModel> AuthController::findOrCreateUserWithInfo(
    const QString &username, const QString &name, const QString &email)
{
    LOG_DEBUG(QString("Finding or creating user with detailed info: %1, %2, %3").arg(username, name, email));

    // Try to find user by email first
    auto user = m_repository->getByEmail(email);
    if (user) {
        LOG_INFO(QString("Found existing user by email: %1").arg(email));

        // Update user name if it's different and not empty
        if (!name.isEmpty() && name != user->name()) {
            LOG_INFO(QString("Updating user name from %1 to %2").arg(user->name(), name));
            user->setName(name);
            m_repository->update(user.data());
        }

        return user;
    }

    // Then try to find by username-based email
    QString defaultEmail = createDefaultEmail(username);
    if (email != defaultEmail) {
        user = m_repository->getByEmail(defaultEmail);
        if (user) {
            LOG_INFO(QString("Found existing user by username: %1").arg(username));

            // Update user info
            bool needsUpdate = false;

            if (!name.isEmpty() && name != user->name()) {
                LOG_INFO(QString("Updating user name from %1 to %2").arg(user->name(), name));
                user->setName(name);
                needsUpdate = true;
            }

            if (email != user->email()) {
                LOG_INFO(QString("Updating user email from %1 to %2").arg(user->email(), email));
                user->setEmail(email);
                needsUpdate = true;
            }

            if (needsUpdate) {
                m_repository->update(user.data());
            }

            return user;
        }
    }

    // Create new user if not found
    return createNewUser(username, name, email);
}

QSharedPointer<UserModel> AuthController::findOrCreateUserFromADInfo(
    const QString &username, const QJsonObject &adUserInfo)
{
    LOG_DEBUG(QString("Finding or creating user from AD info: %1").arg(username));

    // Extract user details from AD response
    QString name = adUserInfo.value("displayName").toString();
    if (name.isEmpty()) {
        name = adUserInfo.value("givenName").toString() + " " + adUserInfo.value("surname").toString();
        if (name.trimmed().isEmpty()) {
            name = username;
        }
    }

    // Get email from AD info or create default
    QString email = adUserInfo.value("email").toString();
    if (email.isEmpty()) {
        email = createDefaultEmail(username);
    }

    return findOrCreateUserWithInfo(username, name, email);
}

QSharedPointer<UserModel> AuthController::createNewUser(
    const QString &username, const QString &name, const QString &email)
{
    LOG_INFO(QString("Creating new user: %1, %2, %3").arg(username, name, email));

    UserModel* newUser = new UserModel();
    newUser->setName(name);
    newUser->setEmail(email);

    // Set a random password - user can set a real one when they log in
    QString randomPassword = QUuid::createUuid().toString().remove("{").remove("}");
    newUser->setPassword(QCryptographicHash::hash(randomPassword.toUtf8(), QCryptographicHash::Sha256).toHex());

    // Mark as active but not verified
    newUser->setActive(true);
    newUser->setVerified(false);

    // Set creation timestamps
    QDateTime now = QDateTime::currentDateTimeUtc();
    newUser->setCreatedAt(now);
    newUser->setUpdatedAt(now);

    // Save the user
    if (m_repository->save(newUser)) {
        LOG_INFO(QString("User created successfully: %1 <%2>").arg(name, email));
        return QSharedPointer<UserModel>(newUser);
    } else {
        LOG_ERROR(QString("Failed to create user: %1 <%2>").arg(name, email));
        delete newUser;
        return nullptr;
    }
}

QString AuthController::createDefaultEmail(const QString &username)
{
    return username + "@" + m_emailDomain;
}

QHttpServerResponse AuthController::processSuccessfulLogin(QSharedPointer<UserModel> user)
{
    // Create user data for token
    QJsonObject userData = userToJson(user.data());

    // Add roles if available - placeholder for role management
    QJsonArray roles;
    // Example: roles would come from a roles repository
    userData["roles"] = roles;

    // Generate tokens using AuthFramework
    QString accessToken = AuthFramework::instance().generateToken(userData);
    QString refreshToken = AuthFramework::instance().generateRefreshToken(userData);

    // Create response
    QJsonObject response;
    response["access_token"] = accessToken;
    response["refresh_token"] = refreshToken;
    response["token_type"] = "Bearer";
    response["expires_in"] = 3600; // 1 hour in seconds
    response["user"] = userData;

    LOG_INFO(QString("User logged in successfully: %1 (%2)")
            .arg(user->name(), user->id().toString()));

    return Http::Response::json(response);
}

bool AuthController::storeActivityRecord(const QUuid &userId, const QJsonObject &activityData)
{
    LOG_DEBUG(QString("Storing activity record for user: %1").arg(userId.toString()));

    // In a real implementation, you would insert this into an activities table
    // For this implementation, we'll just log it
    LOG_INFO(QString("Activity stored: User ID: %1, Type: %2")
             .arg(userId.toString(), activityData["type"].toString()));

    // Simulate database operation
    return true;
}

QString AuthController::generateToken(const QJsonObject &userData)
{
    // Delegate to AuthFramework
    return AuthFramework::instance().generateToken(userData);
}

bool AuthController::removeToken(const QString &token)
{
    // Delegate to AuthFramework
    return AuthFramework::instance().removeToken(token);
}

QString AuthController::generateServiceToken(
    const QString &serviceId,
    const QString &username,
    const QString &computerName,
    const QString &machineId)
{
    // Delegate to AuthFramework
    return AuthFramework::instance().generateServiceToken(
        serviceId, username, computerName, machineId);
}

QString AuthController::getEmailDomain() const
{
    return m_emailDomain;
}

void AuthController::setAutoCreateUsers(bool autoCreate)
{
    m_autoCreateUsers = autoCreate;
    AuthFramework::instance().setAutoCreateUsers(autoCreate);
}

void AuthController::setEmailDomain(const QString &domain)
{
    m_emailDomain = domain;
    AuthFramework::instance().setEmailDomain(domain);
}

QJsonObject AuthController::userToJson(UserModel *user) const
{
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

    return json;
}