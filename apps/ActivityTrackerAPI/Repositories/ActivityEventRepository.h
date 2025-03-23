#ifndef ACTIVITYEVENTREPOSITORY_H
#define ACTIVITYEVENTREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/ActivityEventModel.h"
#include "../Models/EventTypes.h"
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>

class ActivityEventRepository : public BaseRepository<ActivityEventModel>
{
    Q_OBJECT
public:
    explicit ActivityEventRepository(QObject *parent = nullptr);

    // Additional activity event-specific operations
    QList<QSharedPointer<ActivityEventModel>> getBySessionId(const QUuid &sessionId, int limit = 0, int offset = 0);
    QList<QSharedPointer<ActivityEventModel>> getByApplicationId(const QUuid &appId, int limit = 0, int offset = 0);
    QList<QSharedPointer<ActivityEventModel>> getByEventType(
        const QUuid &sessionId,
        EventTypes::ActivityEventType eventType,
        int limit = 0,
        int offset = 0
    );

    QList<QSharedPointer<ActivityEventModel>> getByTimeRange(
        const QUuid &sessionId,
        const QDateTime &startTime,
        const QDateTime &endTime,
        int limit = 0,
        int offset = 0
    );

    // Event count and statistics
    int getEventCountByType(const QUuid &sessionId, EventTypes::ActivityEventType eventType);
    QJsonObject getActivitySummary(const QUuid &sessionId);

protected:
    // BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(ActivityEventModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(ActivityEventModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(ActivityEventModel* model) override;
    ActivityEventModel* createModelFromQuery(const QSqlQuery& query) override;

private:
    // Helper method to convert between string and enum
    QString eventTypeToString(EventTypes::ActivityEventType eventType);
    EventTypes::ActivityEventType stringToEventType(const QString &eventTypeStr);
};

#endif // ACTIVITYEVENTREPOSITORY_H