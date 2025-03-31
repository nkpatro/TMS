#include "SessionEventModel.h"

SessionEventModel::SessionEventModel(QObject *parent)
    : QObject(parent),
      m_eventType(EventTypes::SessionEventType::Login), // Default to avoid uninitialized enum
      m_isRemote(false)
{
    // m_id = QUuid::createUuid();
    m_eventTime = QDateTime::currentDateTimeUtc();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid SessionEventModel::id() const
{
    return m_id;
}

void SessionEventModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QUuid SessionEventModel::sessionId() const
{
    return m_sessionId;
}

void SessionEventModel::setSessionId(const QUuid &sessionId)
{
    if (m_sessionId != sessionId) {
        m_sessionId = sessionId;
        emit sessionIdChanged(m_sessionId);
    }
}

EventTypes::SessionEventType SessionEventModel::eventType() const
{
    return m_eventType;
}

void SessionEventModel::setEventType(EventTypes::SessionEventType eventType)
{
    if (m_eventType != eventType) {
        m_eventType = eventType;
        emit eventTypeChanged(m_eventType);
    }
}

QDateTime SessionEventModel::eventTime() const
{
    return m_eventTime;
}

void SessionEventModel::setEventTime(const QDateTime &eventTime)
{
    if (m_eventTime != eventTime) {
        m_eventTime = eventTime;
        emit eventTimeChanged(m_eventTime);
    }
}

QUuid SessionEventModel::userId() const
{
    return m_userId;
}

void SessionEventModel::setUserId(const QUuid &userId)
{
    if (m_userId != userId) {
        m_userId = userId;
        emit userIdChanged(m_userId);
    }
}

QUuid SessionEventModel::previousUserId() const
{
    return m_previousUserId;
}

void SessionEventModel::setPreviousUserId(const QUuid &previousUserId)
{
    if (m_previousUserId != previousUserId) {
        m_previousUserId = previousUserId;
        emit previousUserIdChanged(m_previousUserId);
    }
}

QUuid SessionEventModel::machineId() const
{
    return m_machineId;
}

void SessionEventModel::setMachineId(const QUuid &machineId)
{
    if (m_machineId != machineId) {
        m_machineId = machineId;
        emit machineIdChanged(m_machineId);
    }
}

QString SessionEventModel::terminalSessionId() const
{
    return m_terminalSessionId;
}

void SessionEventModel::setTerminalSessionId(const QString &terminalSessionId)
{
    if (m_terminalSessionId != terminalSessionId) {
        m_terminalSessionId = terminalSessionId;
        emit terminalSessionIdChanged(m_terminalSessionId);
    }
}

bool SessionEventModel::isRemote() const
{
    return m_isRemote;
}

void SessionEventModel::setIsRemote(bool isRemote)
{
    if (m_isRemote != isRemote) {
        m_isRemote = isRemote;
        emit isRemoteChanged(m_isRemote);
    }
}

QJsonObject SessionEventModel::eventData() const
{
    return m_eventData;
}

void SessionEventModel::setEventData(const QJsonObject &eventData)
{
    if (m_eventData != eventData) {
        m_eventData = eventData;
        emit eventDataChanged(m_eventData);
    }
}

QDateTime SessionEventModel::createdAt() const
{
    return m_createdAt;
}

void SessionEventModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid SessionEventModel::createdBy() const
{
    return m_createdBy;
}

void SessionEventModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime SessionEventModel::updatedAt() const
{
    return m_updatedAt;
}

void SessionEventModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid SessionEventModel::updatedBy() const
{
    return m_updatedBy;
}

void SessionEventModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}
