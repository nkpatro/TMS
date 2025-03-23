// AppUsageModel.cpp
#include "AppUsageModel.h"

AppUsageModel::AppUsageModel(QObject *parent)
    : QObject(parent),
      m_isActive(true)
{
    m_id = QUuid::createUuid();
    m_startTime = QDateTime::currentDateTimeUtc();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid AppUsageModel::id() const
{
    return m_id;
}

void AppUsageModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QUuid AppUsageModel::sessionId() const
{
    return m_sessionId;
}

void AppUsageModel::setSessionId(const QUuid &sessionId)
{
    if (m_sessionId != sessionId) {
        m_sessionId = sessionId;
        emit sessionIdChanged(m_sessionId);
    }
}

QUuid AppUsageModel::appId() const
{
    return m_appId;
}

void AppUsageModel::setAppId(const QUuid &appId)
{
    if (m_appId != appId) {
        m_appId = appId;
        emit appIdChanged(m_appId);
    }
}

QDateTime AppUsageModel::startTime() const
{
    return m_startTime;
}

void AppUsageModel::setStartTime(const QDateTime &startTime)
{
    if (m_startTime != startTime) {
        m_startTime = startTime;
        emit startTimeChanged(m_startTime);
    }
}

QDateTime AppUsageModel::endTime() const
{
    return m_endTime;
}

void AppUsageModel::setEndTime(const QDateTime &endTime)
{
    if (m_endTime != endTime) {
        m_endTime = endTime;
        emit endTimeChanged(m_endTime);
    }
}

bool AppUsageModel::isActive() const
{
    return m_isActive;
}

void AppUsageModel::setIsActive(bool isActive)
{
    if (m_isActive != isActive) {
        m_isActive = isActive;
        emit isActiveChanged(m_isActive);
    }
}

QString AppUsageModel::windowTitle() const
{
    return m_windowTitle;
}

void AppUsageModel::setWindowTitle(const QString &windowTitle)
{
    if (m_windowTitle != windowTitle) {
        m_windowTitle = windowTitle;
        emit windowTitleChanged(m_windowTitle);
    }
}

QDateTime AppUsageModel::createdAt() const
{
    return m_createdAt;
}

void AppUsageModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid AppUsageModel::createdBy() const
{
    return m_createdBy;
}

void AppUsageModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime AppUsageModel::updatedAt() const
{
    return m_updatedAt;
}

void AppUsageModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid AppUsageModel::updatedBy() const
{
    return m_updatedBy;
}

void AppUsageModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}

qint64 AppUsageModel::duration() const
{
    QDateTime end = m_endTime.isValid() ? m_endTime : QDateTime::currentDateTimeUtc();
    return m_startTime.secsTo(end);
}