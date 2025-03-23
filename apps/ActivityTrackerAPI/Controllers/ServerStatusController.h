#ifndef SERVERSTATUSCONTROLLER_H
#define SERVERSTATUSCONTROLLER_H

#include "ApiControllerBase.h"
#include <QDateTime>

/**
 * @brief The ServerStatusController class provides status endpoints for health checking
 *
 * This controller exposes simple endpoints for checking server health and status.
 * It is primarily used by client applications to check connectivity.
 */
class ServerStatusController : public ApiControllerBase
{
    Q_OBJECT
public:
    explicit ServerStatusController(QObject *parent = nullptr);
    ~ServerStatusController() override;

    void setupRoutes(QHttpServer &server) override;
    QString getControllerName() const override { return "ServerStatusController"; }

private:
    QHttpServerResponse handlePingRequest(const QHttpServerRequest &request);
    QHttpServerResponse handleHealthCheck(const QHttpServerRequest &request);
    QHttpServerResponse handleVersionInfo(const QHttpServerRequest &request);

    // Server start time for uptime calculation
    QDateTime m_startTime;

    // Version information
    QString m_version;
    QString m_buildDate;
};

#endif // SERVERSTATUSCONTROLLER_H