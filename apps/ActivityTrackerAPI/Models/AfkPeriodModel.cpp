#include "AfkPeriodModel.h"

AfkPeriodModel::AfkPeriodModel(QObject *parent)
    : QObject(parent)
{
    // m_id = QUuid::createUuid();
    m_startTime = QDateTime::currentDateTimeUtc();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid AfkPeriodModel::id() const
{
    return m_id;
}

void AfkPeriodModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QUuid AfkPeriodModel::sessionId() const
{
    return m_sessionId;
}

void AfkPeriodModel::setSessionId(const QUuid &sessionId)
{
    if (m_sessionId != sessionId) {
        m_sessionId = sessionId;
        emit sessionIdChanged(m_sessionId);
    }
}

QDateTime AfkPeriodModel::startTime() const
{
    return m_startTime;
}

void AfkPeriodModel::setStartTime(const QDateTime &startTime)
{
    if (m_startTime != startTime) {
        m_startTime = startTime;
        emit startTimeChanged(m_startTime);
    }
}

QDateTime AfkPeriodModel::endTime() const
{
    return m_endTime;
}

void AfkPeriodModel::setEndTime(const QDateTime &endTime)
{
    if (m_endTime != endTime) {
        m_endTime = endTime;
        emit endTimeChanged(m_endTime);
    }
}

QDateTime AfkPeriodModel::createdAt() const
{
    return m_createdAt;
}

void AfkPeriodModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid AfkPeriodModel::createdBy() const
{
    return m_createdBy;
}

void AfkPeriodModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime AfkPeriodModel::updatedAt() const
{
    return m_updatedAt;
}

void AfkPeriodModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid AfkPeriodModel::updatedBy() const
{
    return m_updatedBy;
}

void AfkPeriodModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}

bool AfkPeriodModel::isActive() const
{
    return m_endTime.isNull() || !m_endTime.isValid();
}

qint64 AfkPeriodModel::duration() const
{
    QDateTime end = m_endTime.isValid() ? m_endTime : QDateTime::currentDateTimeUtc();
    return m_startTime.secsTo(end);
}
