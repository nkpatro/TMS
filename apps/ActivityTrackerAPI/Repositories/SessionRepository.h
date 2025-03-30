#ifndef SESSIONREPOSITORY_H
#define SESSIONREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/SessionModel.h"
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>

#include "SessionEventRepository.h"

class SessionRepository : public BaseRepository<SessionModel>
{
    Q_OBJECT
public:
    explicit SessionRepository(QObject *parent = nullptr);

    // Additional session-specific operations
    QList<QSharedPointer<SessionModel>> getByUserId(const QUuid &userId, bool activeOnly = false);
    QList<QSharedPointer<SessionModel>> getByMachineId(const QUuid &machineId, bool activeOnly = false);
    QSharedPointer<SessionModel> getActiveSessionForUser(const QUuid &userId, const QUuid &machineId);
    QList<QSharedPointer<SessionModel>> getActiveSessions();

    // Session management
    bool createSessionWithTransaction(SessionModel *session);
    bool safeEndSession(const QUuid &sessionId, const QDateTime &logoutTime = QDateTime::currentDateTimeUtc(), SessionEventRepository* eventRepository = nullptr);
    bool safeReopenSession(const QUuid &sessionId, const QDateTime &updateTime = QDateTime::currentDateTimeUtc(), SessionEventRepository* eventRepository = nullptr);
    bool createSessionLoginEvent(const QUuid &sessionId, const QUuid &userId, const QUuid &machineId, const QDateTime &loginTime, SessionEventRepository* eventRepository, bool isRemote = false, const QString& terminalSessionId = QString());
    bool hasLoginEvent(const QUuid &sessionId, SessionEventRepository* eventRepository);
    bool hasLoginEvent(const QUuid &sessionId, const QDateTime &loginTime, SessionEventRepository* eventRepository);
    bool hasLogoutEvent(const QUuid &sessionId, SessionEventRepository* eventRepository);
    bool hasLogoutEvent(const QUuid &sessionId, const QDateTime &logoutTime, SessionEventRepository* eventRepository);
    bool verifyLoginLogoutPairs(const QUuid &sessionId, SessionEventRepository* eventRepository);

    // Session chain operations
    bool continueSession(const QUuid &previousSessionId, const QUuid &newSessionId);
    QList<QSharedPointer<SessionModel>> getSessionChain(const QUuid &sessionId);
    QJsonObject getSessionChainStats(const QUuid &sessionId);

    // Session analytics
    QJsonObject getUserSessionStats(const QUuid &userId, const QDateTime &startDate, const QDateTime &endDate);

    // Get session for a user/machine for a specific day
    QSharedPointer<SessionModel> getSessionForDay(const QUuid& userId, const QUuid& machineId, const QDate& date);

    // Create or reuse session with transaction support
    QSharedPointer<SessionModel> createOrReuseSessionWithTransaction(
        const QUuid& userId,
        const QUuid& machineId,
        const QDateTime& currentDateTime,
        const QHostAddress& ipAddress,
        const QJsonObject& sessionData,
        bool isRemote = false,
        const QString& terminalSessionId = QString());

    void setSessionEventRepository(SessionEventRepository* sessionEventRepository) { m_sessionEventRepository = sessionEventRepository; }
    bool hasSessionEventRepository() const { return m_sessionEventRepository != nullptr && m_sessionEventRepository->isInitialized(); }

protected:
    // Required BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(SessionModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(SessionModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(SessionModel* model) override;
    SessionModel* createModelFromQuery(const QSqlQuery &query) override;

private:
    SessionEventRepository* m_sessionEventRepository = nullptr;
};

#endif // SESSIONREPOSITORY_H