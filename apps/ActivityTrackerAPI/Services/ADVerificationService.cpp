#include "ADVerificationService.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include "logger/logger.h"

ADVerificationService::ADVerificationService(QObject *parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_adServerUrl("https://ad.redefine.co/api")
{
    LOG_INFO("AD Verification Service initialized");
}

ADVerificationService::~ADVerificationService()
{
    delete m_networkManager;
}

void ADVerificationService::setADServerUrl(const QString &url)
{
    m_adServerUrl = url;
}

// bool ADVerificationService::verifyUserCredentials(const QString &username, const QString &password, QJsonObject &userInfo)
// {
//     LOG_DEBUG(QString("Verifying user credentials with AD: %1").arg(username));
//
//     // Setup AD authentication request
//     QNetworkRequest request(QUrl(m_adServerUrl + "/auth"));
//     request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//
//     // Create request body
//     QJsonObject requestBody;
//     requestBody["username"] = username;
//     requestBody["password"] = password;
//     requestBody["require_password_validation"] = true;
//     QJsonDocument requestDoc(requestBody);
//
//     // Send request
//     QNetworkReply *reply = m_networkManager->post(request, requestDoc.toJson());
//
//     // Wait for response
//     QEventLoop loop;
//     connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
//     loop.exec();
//
//     // Process response
//     bool result = processVerificationResponse(reply, userInfo);
//     reply->deleteLater();
//
//     return result;
// }
//
// bool ADVerificationService::verifyUserExists(const QString &username, QJsonObject &userInfo)
// {
//     LOG_DEBUG(QString("Verifying user exists in AD: %1").arg(username));
//
//     // Setup AD user verification request (no password validation)
//     QNetworkRequest request(QUrl(m_adServerUrl + "/verify-user"));
//     request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//
//     // Create request body - only check if user exists
//     QJsonObject requestBody;
//     requestBody["username"] = username;
//     requestBody["require_password_validation"] = false;  // Don't validate password
//     QJsonDocument requestDoc(requestBody);
//
//     // Send request
//     QNetworkReply *reply = m_networkManager->post(request, requestDoc.toJson());
//
//     // Wait for response
//     QEventLoop loop;
//     connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
//     loop.exec();
//
//     // Process response
//     bool result = processVerificationResponse(reply, userInfo);
//     reply->deleteLater();
//
//     return result;
// }

bool ADVerificationService::verifyUserCredentials(const QString &username, const QString &password, QJsonObject &userInfo)
{
    LOG_DEBUG(QString("MOCK: Verifying user credentials with AD: %1").arg(username));

    // DEVELOPMENT ONLY: Mock successful AD verification
    userInfo["displayName"] = username;
    userInfo["email"] = username + "@redefine.co";
    userInfo["givenName"] = username.split('.').value(0, username);
    userInfo["surname"] = username.split('.').value(1, "");

    LOG_INFO("MOCK: AD verification successful");
    return true;
}

bool ADVerificationService::verifyUserExists(const QString &username, QJsonObject &userInfo)
{
    LOG_DEBUG(QString("MOCK: Verifying user exists in AD: %1").arg(username));

    // DEVELOPMENT ONLY: Mock successful AD verification
    userInfo["displayName"] = username;
    userInfo["email"] = username + "@redefine.co";
    userInfo["givenName"] = username.split('.').value(0, username);
    userInfo["surname"] = username.split('.').value(1, "");

    LOG_INFO("MOCK: AD verification successful");
    return true;
}

bool ADVerificationService::processVerificationResponse(QNetworkReply *reply, QJsonObject &userInfo)
{
    if (reply->error() != QNetworkReply::NoError) {
        LOG_ERROR(QString("AD verification failed: %1").arg(reply->errorString()));
        return false;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    QJsonObject responseObj = responseDoc.object();

    if (responseObj["verified"].toBool()) {
        LOG_INFO("AD verification successful");
        userInfo = responseObj["userInfo"].toObject();
        return true;
    } else {
        LOG_WARNING(QString("AD verification failed: %1").arg(responseObj["message"].toString()));
        return false;
    }
}

bool ADVerificationService::getCachedUserInfo(const QString &username, QJsonObject &userInfo)
{
    // Check if we have cached information for this user
    if (m_userInfoCache.contains(username)) {
        QDateTime cacheTime = m_userInfoCache[username].first;

        // Check if cache is still valid (within 24 hours)
        if (cacheTime.addSecs(24 * 60 * 60) > QDateTime::currentDateTimeUtc()) {
            userInfo = m_userInfoCache[username].second;
            LOG_DEBUG(QString("Using cached AD info for user: %1").arg(username));
            return true;
        } else {
            // Cache expired, remove it
            m_userInfoCache.remove(username);
        }
    }

    return false;
}

void ADVerificationService::cacheUserInfo(const QString &username, const QJsonObject &userInfo)
{
    // Store user info in cache
    m_userInfoCache[username] = qMakePair(QDateTime::currentDateTimeUtc(), userInfo);
    LOG_DEBUG(QString("Cached AD info for user: %1").arg(username));
}

bool ADVerificationService::verifyOrGetCachedUserInfo(const QString &username, QJsonObject &userInfo)
{
    // First check cache
    if (getCachedUserInfo(username, userInfo)) {
        return true;
    }

    try {
        // If not in cache, verify with AD
        bool verified = verifyUserExists(username, userInfo);

        // If verification successful, update cache
        if (verified) {
            cacheUserInfo(username, userInfo);
        }

        return verified;
    }
    catch (const std::exception& e) {
        LOG_ERROR(QString("Exception during AD verification: %1").arg(e.what()));

        // Return fake data in case of connection error to allow system to continue
        userInfo["displayName"] = username;
        userInfo["verified"] = false;
        userInfo["error"] = "Connection to AD failed";

        return false;
    }
}

