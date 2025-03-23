#include "MachineRepository.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QJsonObject>
#include "Core/ModelFactory.h"

MachineRepository::MachineRepository(QObject *parent)
    : BaseRepository<MachineModel>(parent)
{
    LOG_DEBUG("MachineRepository created");
}

QString MachineRepository::getEntityName() const
{
    return "Machine";
}

QString MachineRepository::getModelId(MachineModel* model) const
{
    return model->id().toString();
}

QString MachineRepository::buildSaveQuery()
{
    return "INSERT INTO machines "
           "(name, machine_unique_id, mac_address, operating_system, cpu_info, "
           "gpu_info, ram_size_gb, last_known_ip, last_seen_at, active, "
           "created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:name, :machine_unique_id, :mac_address, :operating_system, :cpu_info, "
           ":gpu_info, :ram_size_gb, :last_known_ip, :last_seen_at, :active::boolean, "
           ":created_at, :created_by, :updated_at, :updated_by) "
           "RETURNING id";
}

QString MachineRepository::buildUpdateQuery()
{
    return "UPDATE machines SET "
           "name = :name, "
           "machine_unique_id = :machine_unique_id, "
           "mac_address = :mac_address, "
           "operating_system = :operating_system, "
           "cpu_info = :cpu_info, "
           "gpu_info = :gpu_info, "
           "ram_size_gb = :ram_size_gb, "
           "last_known_ip = :last_known_ip, "
           "last_seen_at = :last_seen_at, "
           "active = :active::boolean, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE id = :id";
}

QString MachineRepository::buildGetByIdQuery()
{
    return "SELECT * FROM machines WHERE id = :id";
}

QString MachineRepository::buildGetAllQuery()
{
    return "SELECT * FROM machines ORDER BY name";
}

QString MachineRepository::buildRemoveQuery()
{
    return "DELETE FROM machines WHERE id = :id";
}

QMap<QString, QVariant> MachineRepository::prepareParamsForSave(MachineModel* machine)
{
    QMap<QString, QVariant> params;

    // Core fields
    params["name"] = machine->name();

    // If machineUniqueId is empty, generate a unique ID
    if (machine->machineUniqueId().isEmpty()) {
        // Generate a unique ID based on name, mac address and the machine ID
        QString uniqueIdBase = machine->name() + machine->macAddress() + machine->id().toString();
        QByteArray hash = QCryptographicHash::hash(uniqueIdBase.toUtf8(), QCryptographicHash::Sha256);
        QString uniqueId = hash.toHex().left(32);
        machine->setMachineUniqueId(uniqueId);
        LOG_INFO(QString("Generated machine_unique_id: %1 for machine: %2").arg(uniqueId, machine->name()));
    }

    params["machine_unique_id"] = machine->machineUniqueId();
    params["mac_address"] = machine->macAddress();
    params["operating_system"] = machine->operatingSystem();
    params["cpu_info"] = machine->cpuInfo();
    params["gpu_info"] = machine->gpuInfo();
    params["ram_size_gb"] = machine->ramSizeGB();
    params["last_known_ip"] = machine->lastKnownIp();
    params["active"] = machine->active() ? "true" : "false";

    // Nullable field
    params["last_seen_at"] = machine->lastSeenAt().isValid() ?
                             machine->lastSeenAt().toString(Qt::ISODate) :
                             QVariant(QVariant::Invalid);

    // Timestamps and audit fields
    params["created_at"] = machine->createdAt().toString(Qt::ISODate);
    params["created_by"] = machine->createdBy().isNull() ?
                         QVariant(QVariant::Invalid) :
                         machine->createdBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = machine->updatedAt().toString(Qt::ISODate);
    params["updated_by"] = machine->updatedBy().isNull() ?
                         QVariant(QVariant::Invalid) :
                         machine->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

QMap<QString, QVariant> MachineRepository::prepareParamsForUpdate(MachineModel* machine)
{
    QMap<QString, QVariant> params = prepareParamsForSave(machine);

    // Add ID for WHERE clause
    params["id"] = machine->id().toString(QUuid::WithoutBraces);

    return params;
}

MachineModel* MachineRepository::createModelFromQuery(const QSqlQuery& query)
{
    // Use ModelFactory to create the model
    return ModelFactory::createMachineFromQuery(query);
}

QSharedPointer<MachineModel> MachineRepository::getByUniqueId(const QString& uniqueId)
{
    LOG_DEBUG(QString("Getting machine by unique ID: %1").arg(uniqueId));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get machine by unique ID: Repository not initialized");
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["machine_unique_id"] = uniqueId;

    QString query = "SELECT * FROM machines WHERE machine_unique_id = :machine_unique_id";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> MachineModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("Machine found with unique ID: %1 (ID: %2)")
                .arg(uniqueId, (*result)->id().toString()));
        return QSharedPointer<MachineModel>(*result);
    } else {
        LOG_DEBUG(QString("Machine not found with unique ID: %1").arg(uniqueId));
        return nullptr;
    }
}

QSharedPointer<MachineModel> MachineRepository::getByMacAddress(const QString& macAddress)
{
    LOG_DEBUG(QString("Getting machine by MAC address: %1").arg(macAddress));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get machine by MAC address: Repository not initialized");
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["mac_address"] = macAddress;

    QString query = "SELECT * FROM machines WHERE mac_address = :mac_address";

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> MachineModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("Machine found with MAC address: %1 (ID: %2)")
                .arg(macAddress, (*result)->id().toString()));
        return QSharedPointer<MachineModel>(*result);
    } else {
        LOG_DEBUG(QString("Machine not found with MAC address: %1").arg(macAddress));
        return nullptr;
    }
}

QList<QSharedPointer<MachineModel>> MachineRepository::getMachinesByName(const QString& name)
{
    LOG_DEBUG(QString("Getting machines by name: %1").arg(name));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get machines by name: Repository not initialized");
        return QList<QSharedPointer<MachineModel>>();
    }

    QMap<QString, QVariant> params;
    params["name"] = name;

    QString query = "SELECT * FROM machines WHERE name = :name ORDER BY last_seen_at DESC";

    auto machines = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> MachineModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<MachineModel>> result;
    for (auto machine : machines) {
        result.append(QSharedPointer<MachineModel>(machine));
    }

    LOG_INFO(QString("Retrieved %1 machines with name: %2").arg(result.size()).arg(name));
    return result;
}

QList<QSharedPointer<MachineModel>> MachineRepository::getActiveMachines()
{
    LOG_DEBUG("Getting active machines");

    if (!isInitialized()) {
        LOG_ERROR("Cannot get active machines: Repository not initialized");
        return QList<QSharedPointer<MachineModel>>();
    }

    QString query = "SELECT * FROM machines WHERE active = true ORDER BY name";

    auto machines = m_dbService->executeSelectQuery(
        query,
        QMap<QString, QVariant>(),
        [this](const QSqlQuery& query) -> MachineModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<MachineModel>> result;
    for (auto machine : machines) {
        result.append(QSharedPointer<MachineModel>(machine));
    }

    LOG_INFO(QString("Retrieved %1 active machines").arg(result.size()));
    return result;
}

bool MachineRepository::updateLastSeen(const QUuid& id, const QDateTime& timestamp)
{
    LOG_DEBUG(QString("Updating last seen timestamp for machine: %1").arg(id.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot update last seen: Repository not initialized");
        return false;
    }

    QMap<QString, QVariant> params;
    params["id"] = id.toString(QUuid::WithoutBraces);
    params["last_seen_at"] = timestamp.toString(Qt::ISODate);
    params["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QString query =
        "UPDATE machines SET "
        "last_seen_at = :last_seen_at, "
        "updated_at = :updated_at "
        "WHERE id = :id";

    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("Last seen timestamp updated for machine: %1").arg(id.toString()));
    } else {
        LOG_ERROR(QString("Failed to update last seen timestamp for machine: %1 - %2")
                .arg(id.toString(), m_dbService->lastError()));
    }

    return success;
}