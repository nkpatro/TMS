// MachineModel.h
#ifndef MACHINEMODEL_H
#define MACHINEMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>
#include <QHostAddress>

class MachineModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString machineUniqueId READ machineUniqueId WRITE setMachineUniqueId NOTIFY machineUniqueIdChanged)
    Q_PROPERTY(QString macAddress READ macAddress WRITE setMacAddress NOTIFY macAddressChanged)
    Q_PROPERTY(QString operatingSystem READ operatingSystem WRITE setOperatingSystem NOTIFY operatingSystemChanged)
    Q_PROPERTY(QString cpuInfo READ cpuInfo WRITE setCpuInfo NOTIFY cpuInfoChanged)
    Q_PROPERTY(QString gpuInfo READ gpuInfo WRITE setGpuInfo NOTIFY gpuInfoChanged)
    Q_PROPERTY(int ramSizeGB READ ramSizeGB WRITE setRamSizeGB NOTIFY ramSizeGBChanged)
    Q_PROPERTY(QString ipAddress READ ipAddress WRITE setIpAddress NOTIFY ipAddressChanged)
    Q_PROPERTY(QDateTime lastSeenAt READ lastSeenAt WRITE setLastSeenAt NOTIFY lastSeenAtChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)

public:
    explicit MachineModel(QObject *parent = nullptr);

    QUuid id() const;
    void setId(const QUuid &id);

    QString name() const;
    void setName(const QString &name);

    QString machineUniqueId() const;
    void setMachineUniqueId(const QString &machineUniqueId);

    QString macAddress() const;
    void setMacAddress(const QString &macAddress);

    QString operatingSystem() const;
    void setOperatingSystem(const QString &operatingSystem);

    QString cpuInfo() const;
    void setCpuInfo(const QString &cpuInfo);

    QString gpuInfo() const;
    void setGpuInfo(const QString &gpuInfo);

    int ramSizeGB() const;
    void setRamSizeGB(int ramSizeGB);

    QString ipAddress() const;
    void setIpAddress(const QString &ipAddress);

    QDateTime lastSeenAt() const;
    void setLastSeenAt(const QDateTime &lastSeenAt);

    bool active() const;
    void setActive(bool active);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime &createdAt);

    QUuid createdBy() const;
    void setCreatedBy(const QUuid &createdBy);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime &updatedAt);

    QUuid updatedBy() const;
    void setUpdatedBy(const QUuid &updatedBy);

signals:
    void idChanged(const QUuid &id);
    void nameChanged(const QString &name);
    void machineUniqueIdChanged(const QString &machineUniqueId);
    void macAddressChanged(const QString &macAddress);
    void operatingSystemChanged(const QString &operatingSystem);
    void cpuInfoChanged(const QString &cpuInfo);
    void gpuInfoChanged(const QString &gpuInfo);
    void ramSizeGBChanged(int ramSizeGB);
    void ipAddressChanged(const QString &ipAddress);
    void lastSeenAtChanged(const QDateTime &lastSeenAt);
    void activeChanged(bool active);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);

private:
    QUuid m_id;
    QString m_name;
    QString m_machineUniqueId;
    QString m_macAddress;
    QString m_operatingSystem;
    QString m_cpuInfo;
    QString m_gpuInfo;
    int m_ramSizeGB;
    QString m_ipAddress;
    QDateTime m_lastSeenAt;
    bool m_active;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;
};

#endif // MACHINEMODEL_H