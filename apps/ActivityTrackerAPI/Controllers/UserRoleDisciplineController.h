// UserRoleDisciplineController.h
#ifndef USERROLEDISCIPLINECONTROLLER_H
#define USERROLEDISCIPLINECONTROLLER_H

#include "ApiControllerBase.h"
#include "../Repositories/UserRoleDisciplineRepository.h"
#include "AuthController.h"

class UserRoleDisciplineController : public ApiControllerBase
{
    Q_OBJECT
public:
    explicit UserRoleDisciplineController(UserRoleDisciplineRepository *repository, QObject *parent = nullptr);
    explicit UserRoleDisciplineController(UserRoleDisciplineRepository *repository,
                                         AuthController *authController,
                                         QObject *parent = nullptr);

    void setupRoutes(QHttpServer &server) override;
    void setAuthController(AuthController* authController) { m_authController = authController; }
    QString getControllerName() const override { return "UserRoleDisciplineController"; }

private:
    // Endpoint handlers
    QHttpServerResponse handleGetAllAssignments(const QHttpServerRequest &request);
    QHttpServerResponse handleGetUserAssignments(const qint64 userId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetRoleAssignments(const qint64 roleId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetDisciplineAssignments(const qint64 disciplineId, const QHttpServerRequest &request);
    QHttpServerResponse handleAssignRoleDiscipline(const QHttpServerRequest &request);
    QHttpServerResponse handleUpdateAssignment(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleRemoveAssignment(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleCheckAssignment(const QHttpServerRequest &request);

    // Helpers
    QJsonObject assignmentToJson(UserRoleDisciplineModel *model) const;
    QJsonObject extractJsonFromRequest(const QHttpServerRequest &request, bool &ok);
    QUuid stringToUuid(const QString &str) const;
    QString uuidToString(const QUuid &uuid) const;

    UserRoleDisciplineRepository *m_repository;
    AuthController *m_authController = nullptr;
};

#endif // USERROLEDISCIPLINECONTROLLER_H