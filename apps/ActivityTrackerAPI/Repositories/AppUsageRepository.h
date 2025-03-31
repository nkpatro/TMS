#ifndef APPUSAGEREPOSITORY_H
#define APPUSAGEREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/AppUsageModel.h"
#include <QJsonObject>
#include <QJsonArray>

class AppUsageRepository : public BaseRepository<AppUsageModel>
{
    Q_OBJECT
public:
    explicit AppUsageRepository(QObject *parent = nullptr);

    // Additional methods specific to AppUsageRepository
    QList<QSharedPointer<AppUsageModel>> getBySessionId(const QUuid &sessionId);
    QList<QSharedPointer<AppUsageModel>> getByAppId(const QUuid &appId);
    QList<QSharedPointer<AppUsageModel>> getActiveAppUsages(const QUuid &sessionId);
    QSharedPointer<AppUsageModel> getCurrentActiveApp(const QUuid &sessionId);
    bool endAppUsage(const QUuid &usageId, const QDateTime &endTime);
    QJsonObject getAppUsageSummary(const QUuid &sessionId);
    QJsonArray getTopApps(const QUuid &sessionId, int limit = 5);

protected:
    // Required BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(AppUsageModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QString getIdParamName() const override;
    QMap<QString, QVariant> prepareParamsForSave(AppUsageModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(AppUsageModel* model) override;
    AppUsageModel* createModelFromQuery(const QSqlQuery &query) override;
    bool validateModel(AppUsageModel* model, QStringList& errors) override;
    void logQueryWithValues(const QString& query, const QMap<QString, QVariant>& params);
};

#endif // APPUSAGEREPOSITORY_H

