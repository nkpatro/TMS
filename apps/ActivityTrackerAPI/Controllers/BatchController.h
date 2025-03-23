#ifndef BATCHCONTROLLER_H
#define BATCHCONTROLLER_H

#include "ApiControllerBase.h"
#include <QSharedPointer>
#include <QJsonArray>
#include <QJsonObject>

// Include necessary repositories
#include "../Repositories/ActivityEventRepository.h"
#include "../Repositories/AppUsageRepository.h"
#include "../Repositories/SystemMetricsRepository.h"
#include "../Repositories/SessionEventRepository.h"
#include "../Repositories/SessionRepository.h"
#include "AuthController.h"

class BatchController : public ApiControllerBase
{
    Q_OBJECT
public:
    explicit BatchController(QObject *parent = nullptr);
    explicit BatchController(
        ActivityEventRepository* activityRepo,
        AppUsageRepository* appUsageRepo,
        SystemMetricsRepository* metricsRepo,
        SessionEventRepository* sessionEventRepo,
        SessionRepository* sessionRepo,
        QObject *parent = nullptr);
    ~BatchController() override;

    // Initialize the controller
    bool initialize();

    void setupRoutes(QHttpServer &server) override;
    void setAuthController(AuthController* authController) { m_authController = authController; }
    QString getControllerName() const override { return "BatchController"; }

private:
    // Batch processing endpoints
    QHttpServerResponse handleProcessBatch(const QHttpServerRequest &request);
    QHttpServerResponse handleProcessSessionBatch(const qint64 sessionId, const QHttpServerRequest &request);

    // Specific batch processing methods
    bool processActivityEvents(const QJsonArray &events, QUuid sessionId, QUuid userId, QJsonObject &results);
    bool processAppUsages(const QJsonArray &appUsages, QUuid sessionId, QUuid userId, QJsonObject &results);
    bool processSystemMetrics(const QJsonArray &metrics, QUuid sessionId, QUuid userId, QJsonObject &results);
    bool processSessionEvents(const QJsonArray &events, QUuid sessionId, QUuid userId, QJsonObject &results);

    // Helper methods
    QJsonObject extractJsonFromRequest(const QHttpServerRequest &request, bool &ok);
    QUuid stringToUuid(const QString &str) const;
    QString uuidToString(const QUuid &uuid) const;

    // Repository references
    ActivityEventRepository *m_activityEventRepository;
    AppUsageRepository *m_appUsageRepository;
    SystemMetricsRepository *m_systemMetricsRepository;
    SessionEventRepository *m_sessionEventRepository;
    SessionRepository *m_sessionRepository;
    AuthController *m_authController = nullptr;
    bool m_initialized;
};

#endif // BATCHCONTROLLER_H