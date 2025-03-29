// SessionModel.cpp
#include "SessionModel.h"

#include <QJsonDocument>

SessionModel::SessionModel(QObject *parent)
    : QObject(parent),
      m_timeSincePreviousSession(0)
{
    m_id = QUuid::createUuid();
    m_loginTime = QDateTime::currentDateTimeUtc();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid SessionModel::id() const
{
    return m_id;
}

void SessionModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QUuid SessionModel::userId() const
{
    return m_userId;
}

void SessionModel::setUserId(const QUuid &userId)
{
    if (m_userId != userId) {
        m_userId = userId;
        emit userIdChanged(m_userId);
    }
}

QDateTime SessionModel::loginTime() const
{
    return m_loginTime;
}

void SessionModel::setLoginTime(const QDateTime &loginTime)
{
    if (m_loginTime != loginTime) {
        m_loginTime = loginTime;
        emit loginTimeChanged(m_loginTime);
    }
}

QDateTime SessionModel::logoutTime() const
{
    return m_logoutTime;
}

void SessionModel::setLogoutTime(const QDateTime &logoutTime)
{
    if (m_logoutTime != logoutTime) {
        m_logoutTime = logoutTime;
        emit logoutTimeChanged(m_logoutTime);
    }
}

QUuid SessionModel::machineId() const
{
    return m_machineId;
}

void SessionModel::setMachineId(const QUuid &machineId)
{
    if (m_machineId != machineId) {
        m_machineId = machineId;
        emit machineIdChanged(m_machineId);
    }
}

QHostAddress SessionModel::ipAddress() const
{
    return m_ipAddress;
}

void SessionModel::setIpAddress(const QHostAddress &ipAddress)
{
    if (m_ipAddress != ipAddress) {
        m_ipAddress = ipAddress;
        emit ipAddressChanged(m_ipAddress);
    }
}

QJsonObject SessionModel::sessionData() const
{
    return m_sessionData;
}

void SessionModel::setSessionData(const QJsonObject &sessionData)
{
    if (m_sessionData != sessionData) {
        m_sessionData = sessionData;
        emit sessionDataChanged(m_sessionData);
    }
}

QDateTime SessionModel::createdAt() const
{
    return m_createdAt;
}

void SessionModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid SessionModel::createdBy() const
{
    return m_createdBy;
}

void SessionModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime SessionModel::updatedAt() const
{
    return m_updatedAt;
}

void SessionModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid SessionModel::updatedBy() const
{
    return m_updatedBy;
}

void SessionModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}

QUuid SessionModel::continuedFromSession() const
{
    return m_continuedFromSession;
}

void SessionModel::setContinuedFromSession(const QUuid &id)
{
    if (m_continuedFromSession != id) {
        m_continuedFromSession = id;
        emit continuedFromSessionChanged(m_continuedFromSession);
    }
}

QUuid SessionModel::continuedBySession() const
{
    return m_continuedBySession;
}

void SessionModel::setContinuedBySession(const QUuid &id)
{
    if (m_continuedBySession != id) {
        m_continuedBySession = id;
        emit continuedBySessionChanged(m_continuedBySession);
    }
}

QDateTime SessionModel::previousSessionEndTime() const
{
    return m_previousSessionEndTime;
}

void SessionModel::setPreviousSessionEndTime(const QDateTime &endTime)
{
    if (m_previousSessionEndTime != endTime) {
        m_previousSessionEndTime = endTime;
        emit previousSessionEndTimeChanged(m_previousSessionEndTime);
    }
}

qint64 SessionModel::timeSincePreviousSession() const
{
    return m_timeSincePreviousSession;
}

void SessionModel::setTimeSincePreviousSession(qint64 timeInSeconds)
{
    if (m_timeSincePreviousSession != timeInSeconds) {
        m_timeSincePreviousSession = timeInSeconds;
        emit timeSincePreviousSessionChanged(m_timeSincePreviousSession);
    }
}

bool SessionModel::isActive() const
{
    return m_logoutTime.isNull() || !m_logoutTime.isValid();
}

qint64 SessionModel::duration() const
{
    QDateTime endTime = m_logoutTime.isValid() ? m_logoutTime : QDateTime::currentDateTimeUtc();
    return m_loginTime.secsTo(endTime);
}

QString SessionModel::debugInfo() const
{
    QString info = QString("SessionModel [ID: %1]\n").arg(m_id.toString());
    info += QString("  User ID: %1\n").arg(m_userId.toString());
    info += QString("  Machine ID: %1\n").arg(m_machineId.toString());
    info += QString("  Login Time: %1\n").arg(m_loginTime.isValid() ? m_loginTime.toString(Qt::ISODate) : "INVALID");
    info += QString("  Logout Time: %1\n").arg(m_logoutTime.isValid() ? m_logoutTime.toString(Qt::ISODate) : "NULL");
    info += QString("  IP Address: %1\n").arg(m_ipAddress.toString());
    info += QString("  Created At: %1\n").arg(m_createdAt.isValid() ? m_createdAt.toString(Qt::ISODate) : "INVALID");
    info += QString("  Created By: %1\n").arg(m_createdBy.toString());
    info += QString("  Updated At: %1\n").arg(m_updatedAt.isValid() ? m_updatedAt.toString(Qt::ISODate) : "INVALID");
    info += QString("  Updated By: %1\n").arg(m_updatedBy.toString());
    info += QString("  Session Data: %1\n").arg(QString::fromUtf8(QJsonDocument(m_sessionData).toJson()));
    info += QString("  Continued From: %1\n").arg(m_continuedFromSession.toString());
    info += QString("  Continued By: %1\n").arg(m_continuedBySession.toString());
    info += QString("  Previous End Time: %1\n").arg(m_previousSessionEndTime.isValid() ? m_previousSessionEndTime.toString(Qt::ISODate) : "NULL");
    info += QString("  Time Since Previous: %1 seconds").arg(m_timeSincePreviousSession);
    return info;
}