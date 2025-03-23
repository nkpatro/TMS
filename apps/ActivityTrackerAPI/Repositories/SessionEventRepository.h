#ifndef SESSIONEVENTREPOSITORY_H
#define SESSIONEVENTREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/SessionEventModel.h"
#include <QJsonObject>

class SessionEventRepository : public BaseRepository<SessionEventModel>
{
    Q_OBJECT
public:
    explicit SessionEventRepository(QObject *parent = nullptr);

    // Additional methods
    QList<QSharedPointer<SessionEventModel>> getBySessionId(const QUuid &sessionId, int limit = 0, int offset = 0);
    QList<QSharedPointer<SessionEventModel>> getByEventType(
        const QUuid &sessionId,
        EventTypes::SessionEventType eventType,
        int limit = 0,
        int offset = 0
    );
    QList<QSharedPointer<SessionEventModel>> getByTimeRange(
        const QUuid &sessionId,
        const QDateTime &startTime,
        const QDateTime &endTime,
        int limit = 0,
        int offset = 0
    );
    QList<QSharedPointer<SessionEventModel>> getByUserId(const QUuid &userId, int limit = 0, int offset = 0);
    QList<QSharedPointer<SessionEventModel>> getByMachineId(const QString &machineId, int limit = 0, int offset = 0);
    QJsonObject getSessionEventSummary(const QUuid &sessionId);

protected:
    // Required BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(SessionEventModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(SessionEventModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(SessionEventModel* model) override;
    SessionEventModel* createModelFromQuery(const QSqlQuery &query) override;

private:
    // Helper methods
    QString eventTypeToString(EventTypes::SessionEventType eventType);
    EventTypes::SessionEventType stringToEventType(const QString &eventTypeStr);
};

#endif // SESSIONEVENTREPOSITORY_H