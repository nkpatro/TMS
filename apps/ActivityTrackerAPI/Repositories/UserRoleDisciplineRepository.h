#ifndef USERROLEDISCIPLINEREPOSITORY_H
#define USERROLEDISCIPLINEREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/UserRoleDisciplineModel.h"
#include "../Models/RoleModel.h"
#include "../Models/DisciplineModel.h"
#include <QSqlQuery>
#include <QSharedPointer>

#include "DisciplineRepository.h"
#include "RoleRepository.h"
#include "UserRepository.h"

class UserRoleDisciplineRepository : public BaseRepository<UserRoleDisciplineModel>
{
    Q_OBJECT
public:
    explicit UserRoleDisciplineRepository(QObject *parent = nullptr);

    void setUserRepository(UserRepository* userRepository);
    void setRoleRepository(RoleRepository* roleRepository);
    void setDisciplineRepository(DisciplineRepository* disciplineRepository);

    // Additional methods specific to UserRoleDisciplineRepository
    QList<QSharedPointer<UserRoleDisciplineModel>> getByUserId(const QUuid &userId);
    QList<QSharedPointer<UserRoleDisciplineModel>> getByRoleId(const QUuid &roleId);
    QList<QSharedPointer<UserRoleDisciplineModel>> getByDisciplineId(const QUuid &disciplineId);

    // Get user's role in a discipline
    QSharedPointer<UserRoleDisciplineModel> getByUserAndDiscipline(const QUuid &userId, const QUuid &disciplineId);

    // Check if a user has a role in a discipline
    bool userHasRoleInDiscipline(const QUuid &userId, const QUuid &roleId, const QUuid &disciplineId);

    // Get all roles for a user
    QList<QSharedPointer<RoleModel>> getRolesForUser(const QUuid &userId);

    // Get all disciplines for a user
    QList<QSharedPointer<DisciplineModel>> getDisciplinesForUser(const QUuid &userId);

    QList<QSharedPointer<UserModel>> getUsersForRoleInDiscipline(const QUuid &roleId, const QUuid &disciplineId);

    // QList<UserModel*> executeUserSelectQuery(const QString& query, const QMap<QString, QVariant>& params);
    // QList<RoleModel*> executeRoleSelectQuery(const QString& query, const QMap<QString, QVariant>& params);
    // QList<DisciplineModel*> executeDisciplineSelectQuery(const QString& query, const QMap<QString, QVariant>& params);

protected:
    // Required BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getTableName() const override;
    QString getModelId(UserRoleDisciplineModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(UserRoleDisciplineModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(UserRoleDisciplineModel* model) override;
    UserRoleDisciplineModel* createModelFromQuery(const QSqlQuery &query) override;
    bool validateModel(UserRoleDisciplineModel* model, QStringList& errors) override;

private:
    UserRepository* m_userRepository;
    RoleRepository* m_roleRepository;
    DisciplineRepository* m_disciplineRepository;

    // DbService<RoleModel>* m_roleDbService;
    // DbService<DisciplineModel>* m_disciplineDbService;
};

#endif // USERROLEDISCIPLINEREPOSITORY_H

