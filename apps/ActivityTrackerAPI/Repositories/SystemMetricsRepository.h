#ifndef SYSTEMMETRICSREPOSITORY_H
#define SYSTEMMETRICSREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/SystemMetricsModel.h"
#include <QJsonObject>
#include <QJsonArray>

class SystemMetricsRepository : public BaseRepository<SystemMetricsModel>
{
    Q_OBJECT
public:
    explicit SystemMetricsRepository(QObject *parent = nullptr);

    // Additional methods specific to SystemMetricsRepository
    QList<QSharedPointer<SystemMetricsModel>> getBySessionId(const QUuid &sessionId, int limit = 0, int offset = 0);
    QList<QSharedPointer<SystemMetricsModel>> getByTimeRange(
        const QUuid &sessionId,
        const QDateTime &startTime,
        const QDateTime &endTime,
        int limit = 0,
        int offset = 0
    );

    QJsonArray getMetricsTimeSeries(const QUuid &sessionId, const QString &metricType);
    QJsonArray getMetricsTimeSeries(const QUuid &sessionId, const QString &metricType, int limit);
    QJsonObject getAverageMetrics(const QUuid &sessionId);

protected:
    // Required BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getTableName() const override;
    QString getIdParamName() const override;
    QString getModelId(SystemMetricsModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(SystemMetricsModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(SystemMetricsModel* model) override;
    SystemMetricsModel* createModelFromQuery(const QSqlQuery &query) override;
    bool validateModel(SystemMetricsModel* model, QStringList& errors) override;
};

#endif // SYSTEMMETRICSREPOSITORY_H

