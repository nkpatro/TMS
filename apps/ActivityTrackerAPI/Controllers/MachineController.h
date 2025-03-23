#ifndef MACHINECONTROLLER_H
#define MACHINECONTROLLER_H

#include "ApiControllerBase.h"
#include <QSharedPointer>
#include <QList>
#include <QHostAddress>
#include <QJsonObject>
#include <QJsonArray>

#include "Models/MachineModel.h"
#include "Repositories/MachineRepository.h"

class MachineController : public ApiControllerBase
{
    Q_OBJECT

public:
    explicit MachineController(QObject *parent = nullptr);
    explicit MachineController(MachineRepository* repository, QObject *parent = nullptr);
    ~MachineController() override;

    // Initialize the controller
    bool initialize();

    // Setup HTTP routes - required by Http::Controller base class
    void setupRoutes(QHttpServer& server) override;
    QString getControllerName() const override { return "MachineController"; }

    QList<QSharedPointer<MachineModel>> getMachinesByName(const QString &name);

private:
    // HTTP endpoint handlers
    QHttpServerResponse getMachineById(const QString &id);
    QHttpServerResponse getAllMachines(const QHttpServerRequest &request);
    QHttpServerResponse getActiveMachines(const QHttpServerRequest &request);
    QHttpServerResponse createMachine(const QHttpServerRequest &request);
    QHttpServerResponse updateMachine(const QString &id, const QHttpServerRequest &request);
    QHttpServerResponse deleteMachine(const QString &id);
    QHttpServerResponse updateMachineStatus(const QString &id, const QHttpServerRequest &request);
    QHttpServerResponse updateLastSeen(const QString &id, const QHttpServerRequest &request);
    QHttpServerResponse registerMachine(const QHttpServerRequest &request);

    // Helper methods
    QJsonObject machineToJson(const MachineModel *machine) const;
    QJsonArray machinesToJsonArray(const QList<QSharedPointer<MachineModel>> &machines) const;
    bool validateMachineJson(const QJsonObject &json, QStringList &missingFields) const;
    bool isServiceTokenAuthorized(const QHttpServerRequest &request, QJsonObject &userData);

    MachineRepository *m_repository;
    bool m_initialized;
    AuthController* m_authController = nullptr;
};

#endif // MACHINECONTROLLER_H