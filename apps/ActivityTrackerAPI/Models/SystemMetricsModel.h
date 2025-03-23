#ifndef SYSTEMMETRICSMODEL_H
#define SYSTEMMETRICSMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>

class SystemMetricsModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QUuid sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(double cpuUsage READ cpuUsage WRITE setCpuUsage NOTIFY cpuUsageChanged)
    Q_PROPERTY(double gpuUsage READ gpuUsage WRITE setGpuUsage NOTIFY gpuUsageChanged)
    Q_PROPERTY(double memoryUsage READ memoryUsage WRITE setMemoryUsage NOTIFY memoryUsageChanged)
    Q_PROPERTY(QDateTime measurementTime READ measurementTime WRITE setMeasurementTime NOTIFY measurementTimeChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)

public:
    explicit SystemMetricsModel(QObject *parent = nullptr);

    QUuid id() const;
    void setId(const QUuid &id);

    QUuid sessionId() const;
    void setSessionId(const QUuid &sessionId);

    double cpuUsage() const;
    void setCpuUsage(double cpuUsage);

    double gpuUsage() const;
    void setGpuUsage(double gpuUsage);

    double memoryUsage() const;
    void setMemoryUsage(double memoryUsage);

    QDateTime measurementTime() const;
    void setMeasurementTime(const QDateTime &measurementTime);

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
    void sessionIdChanged(const QUuid &sessionId);
    void cpuUsageChanged(double cpuUsage);
    void gpuUsageChanged(double gpuUsage);
    void memoryUsageChanged(double memoryUsage);
    void measurementTimeChanged(const QDateTime &measurementTime);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);

private:
    QUuid m_id;
    QUuid m_sessionId;
    double m_cpuUsage;
    double m_gpuUsage;
    double m_memoryUsage;
    QDateTime m_measurementTime;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;
};

#endif // SYSTEMMETRICSMODEL_H