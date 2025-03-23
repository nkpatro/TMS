#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

#include "BaseRepository.h"
#include "../Models/UserModel.h"
#include <QCryptographicHash>

class UserRepository : public BaseRepository<UserModel>
{
    Q_OBJECT
public:
    explicit UserRepository(QObject *parent = nullptr);

    // Base repository methods - inherited from BaseRepository, but some need customization
    bool save(UserModel *user) override;
    bool update(UserModel *user) override;

    // Additional methods specific to UserRepository
    QSharedPointer<UserModel> getByName(const QString &name);
    QSharedPointer<UserModel> getByEmail(const QString &email);
    QList<QSharedPointer<UserModel>> getActiveUsers();
    bool setUserActive(const QUuid &id, bool active);
    bool setUserVerified(const QUuid &id, bool verified);
    bool validateCredentials(const QString &email, const QString &password, QSharedPointer<UserModel> &user);
    bool updatePassword(const QUuid &id, const QString &newPassword);
    QSharedPointer<UserModel> findOrCreateUser(
        const QString &name,
        const QString &email,
        const QString &password,
        const QUuid &createdBy);

protected:
    // Required BaseRepository abstract method implementations
    QString getEntityName() const override;
    QString getModelId(UserModel* model) const override;
    QString buildSaveQuery() override;
    QString buildUpdateQuery() override;
    QString buildGetByIdQuery() override;
    QString buildGetAllQuery() override;
    QString buildRemoveQuery() override;
    QMap<QString, QVariant> prepareParamsForSave(UserModel* model) override;
    QMap<QString, QVariant> prepareParamsForUpdate(UserModel* model) override;
    UserModel* createModelFromQuery(const QSqlQuery &query) override;

private:
    // Helper methods
    QString hashPassword(const QString &password);
};

#endif // USERREPOSITORY_H