#ifndef SESSIONEVENTCONTROLLER_H
#define SESSIONEVENTCONTROLLER_H

#include "ApiControllerBase.h"
#include <QSharedPointer>
#include "../Models/SessionEventModel.h"
#include "../Models/EventTypes.h"
#include "../Repositories/SessionEventRepository.h"
#include "AuthController.h"

class SessionEventController : public ApiControllerBase
{
    Q_OBJECT
public:
    explicit SessionEventController(QObject *parent = nullptr);
    explicit SessionEventController(SessionEventRepository* repository, QObject *parent = nullptr);
    ~SessionEventController() override;

    // Initialize the controller
    bool initialize();

    void setupRoutes(QHttpServer &server) override;
    void setAuthController(AuthController* authController) { m_authController = authController; }
    QString getControllerName() const override { return "SessionEventController"; }

private:
    // Session event endpoints handlers
    QHttpServerResponse handleGetEvents(const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventById(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventsBySessionId(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventsByEventType(const qint64 sessionId, const QString &eventType, const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventsByTimeRange(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventsByUserId(const qint64 userId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventsByMachineId(const QString &machineId, const QHttpServerRequest &request);
    QHttpServerResponse handleCreateEvent(const QHttpServerRequest &request);
    QHttpServerResponse handleCreateEventForSession(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleUpdateEvent(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleDeleteEvent(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventStats(const qint64 sessionId, const QHttpServerRequest &request);

    // Helpers
    QJsonObject sessionEventToJson(SessionEventModel *event) const;
    QJsonObject extractJsonFromRequest(const QHttpServerRequest &request, bool &ok);
    QUuid stringToUuid(const QString &str) const;
    QString uuidToString(const QUuid &uuid) const;
    EventTypes::SessionEventType stringToEventType(const QString &eventTypeStr) const;
    QString eventTypeToString(EventTypes::SessionEventType eventType) const;

    SessionEventRepository *m_repository;
    bool m_initialized;
    AuthController* m_authController = nullptr;
};

#endif // SESSIONEVENTCONTROLLER_H