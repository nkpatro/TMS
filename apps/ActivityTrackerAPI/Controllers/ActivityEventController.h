#ifndef ACTIVITYEVENTCONTROLLER_H
#define ACTIVITYEVENTCONTROLLER_H

#include "ApiControllerBase.h"
#include <QSharedPointer>
#include "../Models/ActivityEventModel.h"
#include "../Models/EventTypes.h"
#include "../Repositories/ActivityEventRepository.h"
#include "../Repositories/SessionRepository.h"  // Added for session verification
#include "AuthController.h"

class ActivityEventController : public ApiControllerBase
{
    Q_OBJECT
public:
    explicit ActivityEventController(QObject *parent = nullptr);
    explicit ActivityEventController(ActivityEventRepository* repository, AuthController* authController, QObject *parent = nullptr);
    ~ActivityEventController() override;

    // Initialize the controller
    bool initialize();

    void setupRoutes(QHttpServer &server) override;
    void setSessionRepository(SessionRepository* repository) { m_sessionRepository = repository; }
    QString getControllerName() const override { return "ActivityEventController"; }
    void setRepositories(ActivityEventRepository* activityRepository, SessionRepository* sessionRepository);

private:
    // Activity event endpoints handlers
    QHttpServerResponse handleGetEvents(const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventById(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventsBySessionId(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventsByEventType(const qint64 sessionId, const QString &eventType, const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventsByTimeRange(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleCreateEvent(const QHttpServerRequest &request);
    QHttpServerResponse handleCreateEventForSession(const qint64 sessionId, const QHttpServerRequest &request);
    QHttpServerResponse handleUpdateEvent(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleDeleteEvent(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleGetEventStats(const qint64 sessionId, const QHttpServerRequest &request);

    // Helpers
    QJsonObject activityEventToJson(ActivityEventModel *event) const;
    QUuid stringToUuid(const QString &str) const;
    QString uuidToString(const QUuid &uuid) const;
    EventTypes::ActivityEventType stringToEventType(const QString &eventTypeStr);
    QString eventTypeToString(EventTypes::ActivityEventType eventType) const;

    ActivityEventRepository *m_repository;
    SessionRepository *m_sessionRepository;
    AuthController *m_authController;
    bool m_initialized;
};

#endif // ACTIVITYEVENTCONTROLLER_H

