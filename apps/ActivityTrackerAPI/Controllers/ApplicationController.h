#ifndef APPLICATIONCONTROLLER_H
#define APPLICATIONCONTROLLER_H

#include "ApiControllerBase.h"
#include <QSharedPointer>
#include "../Models/ApplicationModel.h"
#include "../Repositories/ApplicationRepository.h"
#include "AuthController.h"

class ApplicationController : public ApiControllerBase
{
    Q_OBJECT
public:
    explicit ApplicationController(QObject *parent = nullptr);
    explicit ApplicationController(ApplicationRepository* repository, QObject *parent = nullptr);
    ~ApplicationController() override;

    // Initialize the controller
    bool initialize();

    // Set auth controller reference
    void setAuthController(std::shared_ptr<AuthController> authController);

    // Implementation of base class methods
    void setupRoutes(QHttpServer &server) override;
    QString getControllerName() const override { return "ApplicationController"; }

private:
    // Application endpoints handlers
    QHttpServerResponse handleGetApplications(const QHttpServerRequest &request);
    QHttpServerResponse handleGetApplicationById(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleCreateApplication(const QHttpServerRequest &request);
    QHttpServerResponse handleUpdateApplication(const qint64 id, const QHttpServerRequest &request);
    QHttpServerResponse handleDeleteApplication(const qint64 id, const QHttpServerRequest &request);

    // Application role and discipline mappings
    QHttpServerResponse handleAssignApplicationToRole(const qint64 appId, const qint64 roleId, const QHttpServerRequest &request);
    QHttpServerResponse handleRemoveApplicationFromRole(const qint64 appId, const qint64 roleId, const QHttpServerRequest &request);
    QHttpServerResponse handleAssignApplicationToDiscipline(const qint64 appId, const qint64 disciplineId, const QHttpServerRequest &request);
    QHttpServerResponse handleRemoveApplicationFromDiscipline(const qint64 appId, const qint64 disciplineId, const QHttpServerRequest &request);

    // Special app collections
    QHttpServerResponse handleGetRestrictedApplications(const QHttpServerRequest &request);
    QHttpServerResponse handleGetTrackedApplications(const QHttpServerRequest &request);
    QHttpServerResponse handleDetectApplication(const QHttpServerRequest &request);
    QHttpServerResponse handleGetApplicationsByRole(const qint64 roleId, const QHttpServerRequest &request);
    QHttpServerResponse handleGetApplicationsByDiscipline(const qint64 disciplineId, const QHttpServerRequest &request);

    // Helpers
    QJsonObject applicationToJson(ApplicationModel *application) const;
    QUuid stringToUuid(const QString &str) const;
    QString uuidToString(const QUuid &uuid) const;

    ApplicationRepository *m_applicationRepository;
    std::shared_ptr<AuthController> m_authController;
    bool m_initialized;
};

#endif // APPLICATIONCONTROLLER_H