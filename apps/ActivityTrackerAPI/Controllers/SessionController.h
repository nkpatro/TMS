#ifndef SESSIONCONTROLLER_H
#define SESSIONCONTROLLER_H

#include "ApiControllerBase.h"
#include <QSharedPointer>
#include "../Models/SessionModel.h"
#include "../Models/ActivityEventModel.h"
#include "../Models/AfkPeriodModel.h"
#include "../Models/SessionEventModel.h"
#include "../Repositories/SessionRepository.h"
#include "../Repositories/ActivityEventRepository.h"
#include "../Repositories/AfkPeriodRepository.h"
#include "../Repositories/AppUsageRepository.h"
#include "../Repositories/MachineRepository.h"
#include "../Repositories/SessionEventRepository.h"
#include "AuthController.h"
#include <QCryptographicHash>

class SessionController : public ApiControllerBase
{
    Q_OBJECT
public:
    explicit SessionController(QObject *parent = nullptr);

    SessionController(
        SessionRepository* sessionRepository,
        ActivityEventRepository* activityEventRepository,
        AfkPeriodRepository* afkPeriodRepository,
        AppUsageRepository* appUsageRepository,
        QObject* parent = nullptr);

    ~SessionController() override;

    // Initialize the controller
    bool initialize();

    void setupRoutes(QHttpServer &server) override;
    void setAuthController(AuthController* authController) { m_authController = authController; }
    void setMachineRepository(MachineRepository* machineRepository) { m_machineRepository = machineRepository; }
    void setSessionEventRepository(SessionEventRepository* sessionEventRepository) { m_sessionEventRepository = sessionEventRepository; }
    QString getControllerName() const override { return "SessionController"; }

private:
    // Session endpoints handlers
    QHttpServerResponse handleGetSessions(const QHttpServerRequest &request);
    QHttpServerResponse handleGetSessionById(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleCreateSession(const QHttpServerRequest &request);
    QHttpServerResponse handleEndSession(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleGetActiveSession(const QHttpServerRequest &request);
    QHttpServerResponse handleGetSessionsByUserId(const qint64 userId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetSessionsByMachineId(const qint64 machineId, const QHttpServerRequest &request);
    QHttpServerResponse handleDayChange(const QUuid& userId, const QUuid& machineId);

    // Activity tracking endpoints
    QHttpServerResponse handleGetSessionActivities(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleRecordActivity(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleStartAfk(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleEndAfk(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetAfkPeriods(const qint64 sessionId, const QHttpServerRequest &request);

    // Session statistics
    QHttpServerResponse handleGetSessionStats(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetUserStats(const qint64 userId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetSessionChain(const qint64 sessionId, const QHttpServerRequest &request);

    // Helper methods
    bool hasOverlappingSession(const QUuid &userId, const QUuid &machineId);
    bool endAllActiveSessions(const QUuid &userId, const QUuid &machineId);
    bool createMachineIfNotFound(const QString &machineName, const QString &machineUniqueId, QUuid &machineId);
    bool recordSessionEvent(
        const QUuid& sessionId,
        EventTypes::SessionEventType eventType,
        const QDateTime& eventTime,
        const QUuid& userId,
        const QUuid& machineId,
        bool isRemote = false,
        const QString& terminalSessionId = QString());

    // JSON helpers
    QJsonObject sessionToJson(SessionModel *session) const;
    QJsonObject afkPeriodToJson(AfkPeriodModel *afkPeriod) const;
    QJsonObject activityEventToJson(ActivityEventModel *event) const;
    QJsonObject extractJsonFromRequest(const QHttpServerRequest &request, bool &ok);

    // Repository references
    SessionRepository *m_repository;
    ActivityEventRepository *m_activityEventRepository;
    AfkPeriodRepository *m_afkPeriodRepository;
    AppUsageRepository *m_appUsageRepository;
    MachineRepository *m_machineRepository;
    SessionEventRepository *m_sessionEventRepository;
    bool m_initialized;
    AuthController* m_authController = nullptr;
};

#endif // SESSIONCONTROLLER_H

