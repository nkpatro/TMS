#ifndef ROLEREPOSITORY_H
#define ROLEREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/RoleModel.h"
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>

class RoleRepository : public BaseRepository<RoleModel>
{
    Q_OBJECT
public:
    explicit RoleRepository(QObject *parent = nullptr);

    // Override remove to check references before deletion
    bool remove(const QUuid &id) override;

    // Additional query methods
    QSharedPointer<RoleModel> getByCode(const QString &code);
    QList<QSharedPointer<RoleModel>> getByName(const QString &name);
    // QList<QSharedPointer<RoleModel>> search(const QString &term);
    QList<QSharedPointer<RoleModel>> searchRoles(const QString& term, int limit = 50);

    // Batch operations
    bool batchCreate(const QList<RoleModel*> &roles);

    // Helper methods
    QSharedPointer<RoleModel> getOrCreate(const QString &code, const QString &name, const QString &description = "");

    // Statistics
    QJsonObject getRoleStats(const QUuid &id);

protected:
    // Required BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(RoleModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(RoleModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(RoleModel* model) override;
    RoleModel* createModelFromQuery(const QSqlQuery &query) override;
};

#endif // ROLEREPOSITORY_H

