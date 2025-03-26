// AuthController.h
#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include "ApiControllerBase.h"
#include <QSharedPointer>
#include "../Models/UserModel.h"
#include "../Repositories/UserRepository.h"

class ADVerificationService;

class AuthController : public ApiControllerBase
{
    Q_OBJECT
public:
    explicit AuthController(UserRepository *userRepository,
                          ADVerificationService *adService,
                          QObject *parent = nullptr);

    void setupRoutes(QHttpServer &server) override;
    QString getControllerName() const override { return "AuthController"; }

    // Public authentication methods for other controllers to use
    bool isUserAuthorized(const QHttpServerRequest &request, QJsonObject &userData, bool strictMode = false);

    // Service token methods
    QString extractServiceToken(const QHttpServerRequest &request);
    bool validateServiceToken(const QString &token, QJsonObject &tokenData);

    // New methods for non-interactive user tracking
    QSharedPointer<UserModel> validateAndGetUserForTracking(const QString &username);
    QSharedPointer<UserModel> findOrCreateUserByUsername(const QString &username);
    QSharedPointer<UserModel> findOrCreateUserWithInfo(const QString &username, const QString &name, const QString &email);
    QSharedPointer<UserModel> findOrCreateUserFromADInfo(const QString &username, const QJsonObject &adUserInfo);
    QString getEmailDomain() const;

    // Configuration methods
    void setAutoCreateUsers(bool autoCreate);
    void setEmailDomain(const QString &domain);
    QUuid createDefaultAdminUser();

private:
    // Auth endpoints handlers
    QHttpServerResponse handleLogin(const QHttpServerRequest &request);
    QHttpServerResponse handleLogout(const QHttpServerRequest &request);
    QHttpServerResponse handleGetProfile(const QHttpServerRequest &request);
    QHttpServerResponse handleRefreshToken(const QHttpServerRequest &request);
    QHttpServerResponse handleActivityTracking(const QHttpServerRequest &request);
    QHttpServerResponse handleServiceToken(const QHttpServerRequest &request);

    // User creation and validation
    QSharedPointer<UserModel> createNewUser(const QString &username, const QString &name, const QString &email);

    // Helper methods
    QString createDefaultEmail(const QString &username);
    QHttpServerResponse processSuccessfulLogin(QSharedPointer<UserModel> user);
    bool storeActivityRecord(const QUuid &userId, const QJsonObject &activityData);

    // Token management
    QString generateToken(const QJsonObject &userData);
    bool removeToken(const QString &token);
    QString generateServiceToken(const QString &serviceId, const QString &username,
                                const QString &computerName, const QString &machineId);

    // Helpers
    QJsonObject userToJson(UserModel *user) const;

    UserRepository *m_repository;
    ADVerificationService *m_adService;

    // Configuration parameters
    bool m_autoCreateUsers;
    QString m_emailDomain;

    // In-memory token storage (would use a more robust solution in production)
    QMap<QString, QJsonObject> m_tokenToUserData;
    QMap<QString, QJsonObject> m_serviceTokens;
};

#endif // AUTHCONTROLLER_H

