#include "ActivityEventModel.h"

ActivityEventModel::ActivityEventModel(QObject *parent)
    : QObject(parent),
      m_eventType(EventTypes::ActivityEventType::MouseClick) // Default to avoid uninitialized enum
{
    // m_id = QUuid::createUuid();
    m_eventTime = QDateTime::currentDateTimeUtc();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid ActivityEventModel::id() const
{
    return m_id;
}

void ActivityEventModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QUuid ActivityEventModel::sessionId() const
{
    return m_sessionId;
}

void ActivityEventModel::setSessionId(const QUuid &sessionId)
{
    if (m_sessionId != sessionId) {
        m_sessionId = sessionId;
        emit sessionIdChanged(m_sessionId);
    }
}

QUuid ActivityEventModel::appId() const
{
    return m_appId;
}

void ActivityEventModel::setAppId(const QUuid &appId)
{
    if (m_appId != appId) {
        m_appId = appId;
        emit appIdChanged(m_appId);
    }
}

EventTypes::ActivityEventType ActivityEventModel::eventType() const
{
    return m_eventType;
}

void ActivityEventModel::setEventType(EventTypes::ActivityEventType eventType)
{
    if (m_eventType != eventType) {
        m_eventType = eventType;
        emit eventTypeChanged(m_eventType);
    }
}

QDateTime ActivityEventModel::eventTime() const
{
    return m_eventTime;
}

void ActivityEventModel::setEventTime(const QDateTime &eventTime)
{
    if (m_eventTime != eventTime) {
        m_eventTime = eventTime;
        emit eventTimeChanged(m_eventTime);
    }
}

QJsonObject ActivityEventModel::eventData() const
{
    return m_eventData;
}

void ActivityEventModel::setEventData(const QJsonObject &eventData)
{
    if (m_eventData != eventData) {
        m_eventData = eventData;
        emit eventDataChanged(m_eventData);
    }
}

QDateTime ActivityEventModel::createdAt() const
{
    return m_createdAt;
}

void ActivityEventModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid ActivityEventModel::createdBy() const
{
    return m_createdBy;
}

void ActivityEventModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime ActivityEventModel::updatedAt() const
{
    return m_updatedAt;
}

void ActivityEventModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid ActivityEventModel::updatedBy() const
{
    return m_updatedBy;
}

void ActivityEventModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}