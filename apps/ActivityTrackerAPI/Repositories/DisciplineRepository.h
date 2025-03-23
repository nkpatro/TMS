#ifndef DISCIPLINEREPOSITORY_H
#define DISCIPLINEREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/DisciplineModel.h"
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>

class DisciplineRepository : public BaseRepository<DisciplineModel>
{
    Q_OBJECT
public:
    explicit DisciplineRepository(QObject *parent = nullptr);

    // Override remove to check references before deletion
    bool remove(const QUuid &id) override;

    // Additional query methods
    QSharedPointer<DisciplineModel> getByCode(const QString &code);
    QList<QSharedPointer<DisciplineModel>> getByName(const QString &name);
    QList<QSharedPointer<DisciplineModel>> search(const QString &term);

    // Batch operations
    bool batchCreate(const QList<DisciplineModel*> &disciplines);

    // Helper methods
    QSharedPointer<DisciplineModel> getOrCreate(const QString &code, const QString &name, const QString &description = "");

    // Statistics
    QJsonObject getDisciplineStats(const QUuid &id);

protected:
    // Required BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(DisciplineModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(DisciplineModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(DisciplineModel* model) override;
    DisciplineModel* createModelFromQuery(const QSqlQuery &query) override;
};

#endif // DISCIPLINEREPOSITORY_H

