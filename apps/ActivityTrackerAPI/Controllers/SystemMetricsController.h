#ifndef SYSTEMMETRICSCONTROLLER_H
#define SYSTEMMETRICSCONTROLLER_H

#include "ApiControllerBase.h"
#include <QSharedPointer>

#include "AuthController.h"
#include "../Models/SystemMetricsModel.h"
#include "../Repositories/SystemMetricsRepository.h"

class SystemMetricsController : public ApiControllerBase
{
    Q_OBJECT
public:
    explicit SystemMetricsController(QObject *parent = nullptr);
    explicit SystemMetricsController(SystemMetricsRepository* repository, QObject *parent = nullptr);
    ~SystemMetricsController() override;

    // Initialize the controller
    bool initialize();

    void setupRoutes(QHttpServer &server) override;
    void setAuthController(AuthController* authController) { m_authController = authController; }
    QString getControllerName() const override { return "SystemMetricsController"; }

private:
    // System metrics endpoints handlers
    QHttpServerResponse handleGetMetrics(const QHttpServerRequest &request);
    QHttpServerResponse handleGetMetricsBySessionId(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleRecordMetrics(const QHttpServerRequest &request);
    QHttpServerResponse handleRecordMetricsForSession(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetAverageMetrics(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetMetricsTimeSeries(const qint64 sessionId, const QString &metricType, const QHttpServerRequest &request);
    QHttpServerResponse handleGetSystemInfo(const QHttpServerRequest &request);

    // Helpers
    QJsonObject systemMetricsToJson(SystemMetricsModel *metrics) const;
    QJsonObject extractJsonFromRequest(const QHttpServerRequest &request, bool &ok);
    QUuid stringToUuid(const QString &str) const;
    QString uuidToString(const QUuid &uuid) const;

    SystemMetricsRepository *m_systemMetricsRepository;
    bool m_initialized;
    AuthController* m_authController = nullptr;
};

#endif // SYSTEMMETRICSCONTROLLER_H