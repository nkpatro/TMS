#ifndef MACHINEREPOSITORY_H
#define MACHINEREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/MachineModel.h"
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <QDateTime>

class MachineRepository : public BaseRepository<MachineModel>
{
    Q_OBJECT
public:
    explicit MachineRepository(QObject *parent = nullptr);

    // BaseRepository implementation
    QSharedPointer<MachineModel> getByUniqueId(const QString& uniqueId);
    QSharedPointer<MachineModel> getByMacAddress(const QString& macAddress);
    QSharedPointer<MachineModel> getMachineByName(const QString& name);
    QList<QSharedPointer<MachineModel>> getActiveMachines();
    bool updateLastSeen(const QUuid& id, const QDateTime& timestamp = QDateTime::currentDateTimeUtc());

protected:
    // Required BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(MachineModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(MachineModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(MachineModel* model) override;
    MachineModel* createModelFromQuery(const QSqlQuery& query) override;
};

#endif // MACHINEREPOSITORY_H