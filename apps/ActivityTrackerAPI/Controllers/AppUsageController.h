#ifndef APPUSAGECONTROLLER_H
#define APPUSAGECONTROLLER_H

#include "ApiControllerBase.h"
#include <QSharedPointer>

#include "AuthController.h"
#include "../Models/AppUsageModel.h"
#include "../Repositories/AppUsageRepository.h"
#include "../Repositories/ApplicationRepository.h"

class AppUsageController : public ApiControllerBase
{
    Q_OBJECT
public:
    explicit AppUsageController(QObject *parent = nullptr);
    explicit AppUsageController(AppUsageRepository* appUsageRepository,
                               ApplicationRepository* applicationRepository,
                               QObject *parent = nullptr);
    ~AppUsageController() override;

    // Initialize the controller
    bool initialize();

    void setupRoutes(QHttpServer &server) override;
    QString getControllerName() const override { return "AppUsageController"; }
    void setAuthController(AuthController* authController) { m_authController = authController; }

private:
    // App usage endpoints handlers
    QHttpServerResponse handleGetAppUsages(const QHttpServerRequest &request);
    QHttpServerResponse handleGetAppUsageById(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleGetAppUsagesBySessionId(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetAppUsagesByAppId(const qint64 appId, const QHttpServerRequest &request);
    QHttpServerResponse handleStartAppUsage(const QHttpServerRequest &request);
    QHttpServerResponse handleEndAppUsage(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleUpdateAppUsage(const qint64 id, const QHttpServerRequest &request);

    // App usage statistics
    QHttpServerResponse handleGetSessionAppUsageStats(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetTopApps(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetActiveApps(const qint64 sessionId, const QHttpServerRequest &request);

    // Helpers
    QJsonObject appUsageToJson(AppUsageModel *appUsage) const;
    QUuid stringToUuid(const QString &str) const;
    QString uuidToString(const QUuid &uuid) const;

    AppUsageRepository *m_appUsageRepository;
    ApplicationRepository *m_applicationRepository;
    bool m_initialized;
    AuthController* m_authController = nullptr;
};

#endif // APPUSAGECONTROLLER_H