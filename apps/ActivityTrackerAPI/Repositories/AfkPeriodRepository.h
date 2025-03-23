#ifndef AFKPERIODREPOSITORY_H
#define AFKPERIODREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/AfkPeriodModel.h"
#include <QJsonObject>

class AfkPeriodRepository : public BaseRepository<AfkPeriodModel>
{
    Q_OBJECT
public:
    explicit AfkPeriodRepository(QObject *parent = nullptr);

    // Additional methods specific to AfkPeriodRepository
    QList<QSharedPointer<AfkPeriodModel>> getBySessionId(const QUuid &sessionId);
    QList<QSharedPointer<AfkPeriodModel>> getActiveAfkPeriods(const QUuid &sessionId);
    QSharedPointer<AfkPeriodModel> getLastAfkPeriod(const QUuid &sessionId);
    bool endAfkPeriod(const QUuid &afkId, const QDateTime &endTime);
    QJsonObject getAfkSummary(const QUuid &sessionId);

protected:
    // BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(AfkPeriodModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(AfkPeriodModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(AfkPeriodModel* model) override;
    AfkPeriodModel* createModelFromQuery(const QSqlQuery &query) override;

    QString getIdParamName() const override { return "afk_id"; }
};

#endif // AFKPERIODREPOSITORY_H