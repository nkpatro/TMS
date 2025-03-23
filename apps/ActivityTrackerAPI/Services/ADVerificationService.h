#ifndef ADVERIFICATIONSERVICE_H
#define ADVERIFICATIONSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QDateTime>
#include <QPair>
#include <QMap>

class ADVerificationService : public QObject
{
    Q_OBJECT
public:
    explicit ADVerificationService(QObject *parent = nullptr);
    ~ADVerificationService();

    // Set AD server URL
    void setADServerUrl(const QString &url);

    // Full verification with username and password
    bool verifyUserCredentials(const QString &username, const QString &password, QJsonObject &userInfo);

    // Verify user exists in AD (no password validation)
    bool verifyUserExists(const QString &username, QJsonObject &userInfo);

    // Get user info from cache or verify with AD if not cached
    bool verifyOrGetCachedUserInfo(const QString &username, QJsonObject &userInfo);

private:
    // Process verification response
    bool processVerificationResponse(QNetworkReply *reply, QJsonObject &userInfo);

    // User info caching methods
    bool getCachedUserInfo(const QString &username, QJsonObject &userInfo);
    void cacheUserInfo(const QString &username, const QJsonObject &userInfo);

    QNetworkAccessManager *m_networkManager;
    QString m_adServerUrl;

    // Cache of user info: username -> (timestamp, user info)
    QMap<QString, QPair<QDateTime, QJsonObject>> m_userInfoCache;
};

#endif // ADVERIFICATIONSERVICE_H