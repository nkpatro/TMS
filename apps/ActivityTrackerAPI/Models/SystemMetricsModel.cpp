// SystemMetricsModel.cpp
#include "SystemMetricsModel.h"

SystemMetricsModel::SystemMetricsModel(QObject *parent)
    : QObject(parent),
      m_cpuUsage(0.0),
      m_gpuUsage(0.0),
      m_memoryUsage(0.0)
{
    // m_id = QUuid::createUuid();
    m_measurementTime = QDateTime::currentDateTimeUtc();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid SystemMetricsModel::id() const
{
    return m_id;
}

void SystemMetricsModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QUuid SystemMetricsModel::sessionId() const
{
    return m_sessionId;
}

void SystemMetricsModel::setSessionId(const QUuid &sessionId)
{
    if (m_sessionId != sessionId) {
        m_sessionId = sessionId;
        emit sessionIdChanged(m_sessionId);
    }
}

double SystemMetricsModel::cpuUsage() const
{
    return m_cpuUsage;
}

void SystemMetricsModel::setCpuUsage(double cpuUsage)
{
    if (m_cpuUsage != cpuUsage) {
        m_cpuUsage = cpuUsage;
        emit cpuUsageChanged(m_cpuUsage);
    }
}

double SystemMetricsModel::gpuUsage() const
{
    return m_gpuUsage;
}

void SystemMetricsModel::setGpuUsage(double gpuUsage)
{
    if (m_gpuUsage != gpuUsage) {
        m_gpuUsage = gpuUsage;
        emit gpuUsageChanged(m_gpuUsage);
    }
}

double SystemMetricsModel::memoryUsage() const
{
    return m_memoryUsage;
}

void SystemMetricsModel::setMemoryUsage(double memoryUsage)
{
    if (m_memoryUsage != memoryUsage) {
        m_memoryUsage = memoryUsage;
        emit memoryUsageChanged(m_memoryUsage);
    }
}

QDateTime SystemMetricsModel::measurementTime() const
{
    return m_measurementTime;
}

void SystemMetricsModel::setMeasurementTime(const QDateTime &measurementTime)
{
    if (m_measurementTime != measurementTime) {
        m_measurementTime = measurementTime;
        emit measurementTimeChanged(m_measurementTime);
    }
}

QDateTime SystemMetricsModel::createdAt() const
{
    return m_createdAt;
}

void SystemMetricsModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid SystemMetricsModel::createdBy() const
{
    return m_createdBy;
}

void SystemMetricsModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime SystemMetricsModel::updatedAt() const
{
    return m_updatedAt;
}

void SystemMetricsModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid SystemMetricsModel::updatedBy() const
{
    return m_updatedBy;
}

void SystemMetricsModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}