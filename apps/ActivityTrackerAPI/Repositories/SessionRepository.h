#ifndef SESSIONREPOSITORY_H
#define SESSIONREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/SessionModel.h"
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>

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
    bool endSession(const QUuid &sessionId, const QDateTime &logoutTime = QDateTime::currentDateTimeUtc());

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
};

#endif // SESSIONREPOSITORY_H