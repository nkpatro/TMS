#ifndef APPLICATIONREPOSITORY_H
#define APPLICATIONREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/ApplicationModel.h"
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <QDateTime>

class ApplicationRepository : public BaseRepository<ApplicationModel>
{
    Q_OBJECT
public:
    explicit ApplicationRepository(QObject *parent = nullptr);

    // Additional application-specific operations
    QSharedPointer<ApplicationModel> getByPath(const QString &appPath);
    QSharedPointer<ApplicationModel> getByPathAndName(const QString &appPath, const QString &appName);
    QList<QSharedPointer<ApplicationModel>> getByRoleId(const QUuid &roleId);
    QList<QSharedPointer<ApplicationModel>> getByDisciplineId(const QUuid &disciplineId);
    QList<QSharedPointer<ApplicationModel>> getTrackedApplications();
    QList<QSharedPointer<ApplicationModel>> getRestrictedApplications();

    // Application tracking operations
    bool assignApplicationToRole(const QUuid &appId, const QUuid &roleId, const QUuid &userId);
    bool removeApplicationFromRole(const QUuid &appId, const QUuid &roleId);
    bool assignApplicationToDiscipline(const QUuid &appId, const QUuid &disciplineId, const QUuid &userId);
    bool removeApplicationFromDiscipline(const QUuid &appId, const QUuid &disciplineId);

    // Helper methods for finding or creating applications
    QSharedPointer<ApplicationModel> findOrCreateApplication(
        const QString &appName,
        const QString &appPath,
        const QString &appHash,
        bool isRestricted,
        bool trackingEnabled,
        const QUuid &createdBy
    );

protected:
    // BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(ApplicationModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(ApplicationModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(ApplicationModel* model) override;
    ApplicationModel* createModelFromQuery(const QSqlQuery &query) override;
    void logQueryWithValues(const QString& query, const QMap<QString, QVariant>& params);
};

#endif // APPLICATIONREPOSITORY_H

