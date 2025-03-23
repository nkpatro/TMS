// MachineModel.cpp
#include "MachineModel.h"

MachineModel::MachineModel(QObject *parent)
    : QObject(parent),
      m_ramSizeGB(0),
      m_active(true)
{
    m_id = QUuid::createUuid();
    m_lastSeenAt = QDateTime::currentDateTimeUtc();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid MachineModel::id() const
{
    return m_id;
}

void MachineModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QString MachineModel::name() const
{
    return m_name;
}

void MachineModel::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
    }
}

QString MachineModel::machineUniqueId() const
{
    return m_machineUniqueId;
}

void MachineModel::setMachineUniqueId(const QString &machineUniqueId)
{
    if (m_machineUniqueId != machineUniqueId) {
        m_machineUniqueId = machineUniqueId;
        emit machineUniqueIdChanged(m_machineUniqueId);
    }
}

QString MachineModel::macAddress() const
{
    return m_macAddress;
}

void MachineModel::setMacAddress(const QString &macAddress)
{
    if (m_macAddress != macAddress) {
        m_macAddress = macAddress;
        emit macAddressChanged(m_macAddress);
    }
}

QString MachineModel::operatingSystem() const
{
    return m_operatingSystem;
}

void MachineModel::setOperatingSystem(const QString &operatingSystem)
{
    if (m_operatingSystem != operatingSystem) {
        m_operatingSystem = operatingSystem;
        emit operatingSystemChanged(m_operatingSystem);
    }
}

QString MachineModel::cpuInfo() const
{
    return m_cpuInfo;
}

void MachineModel::setCpuInfo(const QString &cpuInfo)
{
    if (m_cpuInfo != cpuInfo) {
        m_cpuInfo = cpuInfo;
        emit cpuInfoChanged(m_cpuInfo);
    }
}

QString MachineModel::gpuInfo() const
{
    return m_gpuInfo;
}

void MachineModel::setGpuInfo(const QString &gpuInfo)
{
    if (m_gpuInfo != gpuInfo) {
        m_gpuInfo = gpuInfo;
        emit gpuInfoChanged(m_gpuInfo);
    }
}

int MachineModel::ramSizeGB() const
{
    return m_ramSizeGB;
}

void MachineModel::setRamSizeGB(int ramSizeGB)
{
    if (m_ramSizeGB != ramSizeGB) {
        m_ramSizeGB = ramSizeGB;
        emit ramSizeGBChanged(m_ramSizeGB);
    }
}

QString MachineModel::lastKnownIp() const
{
    return m_lastKnownIp;
}

void MachineModel::setLastKnownIp(const QString &lastKnownIp)
{
    if (m_lastKnownIp != lastKnownIp) {
        m_lastKnownIp = lastKnownIp;
        emit lastKnownIpChanged(m_lastKnownIp);
    }
}

QDateTime MachineModel::lastSeenAt() const
{
    return m_lastSeenAt;
}

void MachineModel::setLastSeenAt(const QDateTime &lastSeenAt)
{
    if (m_lastSeenAt != lastSeenAt) {
        m_lastSeenAt = lastSeenAt;
        emit lastSeenAtChanged(m_lastSeenAt);
    }
}

bool MachineModel::active() const
{
    return m_active;
}

void MachineModel::setActive(bool active)
{
    if (m_active != active) {
        m_active = active;
        emit activeChanged(m_active);
    }
}

QDateTime MachineModel::createdAt() const
{
    return m_createdAt;
}

void MachineModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid MachineModel::createdBy() const
{
    return m_createdBy;
}

void MachineModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime MachineModel::updatedAt() const
{
    return m_updatedAt;
}

void MachineModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid MachineModel::updatedBy() const
{
    return m_updatedBy;
}

void MachineModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}