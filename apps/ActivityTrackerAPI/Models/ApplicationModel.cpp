#include "ApplicationModel.h"

ApplicationModel::ApplicationModel(QObject *parent)
    : QObject(parent),
      m_isRestricted(false),
      m_trackingEnabled(true)
{
    m_id = QUuid::createUuid();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid ApplicationModel::id() const
{
    return m_id;
}

void ApplicationModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QString ApplicationModel::appName() const
{
    return m_appName;
}

void ApplicationModel::setAppName(const QString &appName)
{
    if (m_appName != appName) {
        m_appName = appName;
        emit appNameChanged(m_appName);
    }
}

QString ApplicationModel::appPath() const
{
    return m_appPath;
}

void ApplicationModel::setAppPath(const QString &appPath)
{
    if (m_appPath != appPath) {
        m_appPath = appPath;
        emit appPathChanged(m_appPath);
    }
}

QString ApplicationModel::appHash() const
{
    return m_appHash;
}

void ApplicationModel::setAppHash(const QString &appHash)
{
    if (m_appHash != appHash) {
        m_appHash = appHash;
        emit appHashChanged(m_appHash);
    }
}

bool ApplicationModel::isRestricted() const
{
    return m_isRestricted;
}

void ApplicationModel::setIsRestricted(bool isRestricted)
{
    if (m_isRestricted != isRestricted) {
        m_isRestricted = isRestricted;
        emit isRestrictedChanged(m_isRestricted);
    }
}

bool ApplicationModel::trackingEnabled() const
{
    return m_trackingEnabled;
}

void ApplicationModel::setTrackingEnabled(bool trackingEnabled)
{
    if (m_trackingEnabled != trackingEnabled) {
        m_trackingEnabled = trackingEnabled;
        emit trackingEnabledChanged(m_trackingEnabled);
    }
}

QDateTime ApplicationModel::createdAt() const
{
    return m_createdAt;
}

void ApplicationModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid ApplicationModel::createdBy() const
{
    return m_createdBy;
}

void ApplicationModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime ApplicationModel::updatedAt() const
{
    return m_updatedAt;
}

void ApplicationModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid ApplicationModel::updatedBy() const
{
    return m_updatedBy;
}

void ApplicationModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}