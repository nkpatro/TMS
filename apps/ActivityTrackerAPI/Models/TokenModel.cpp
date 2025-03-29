#include "TokenModel.h"

TokenModel::TokenModel(QObject *parent) : QObject(parent), m_revoked(false)
{
    // Initialize with default values
    m_id = QUuid::createUuid();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
    m_lastUsedAt = m_createdAt;

    // Default expiry to one day from now
    m_expiresAt = m_createdAt.addDays(1);
}

QUuid TokenModel::id() const
{
    return m_id;
}

void TokenModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QString TokenModel::tokenId() const
{
    return m_tokenId;
}

void TokenModel::setTokenId(const QString &tokenId)
{
    if (m_tokenId != tokenId) {
        m_tokenId = tokenId;
        emit tokenIdChanged(m_tokenId);
    }
}

QString TokenModel::tokenType() const
{
    return m_tokenType;
}

void TokenModel::setTokenType(const QString &tokenType)
{
    if (m_tokenType != tokenType) {
        m_tokenType = tokenType;
        emit tokenTypeChanged(m_tokenType);
    }
}

QUuid TokenModel::userId() const
{
    return m_userId;
}

void TokenModel::setUserId(const QUuid &userId)
{
    if (m_userId != userId) {
        m_userId = userId;
        emit userIdChanged(m_userId);
    }
}

QJsonObject TokenModel::tokenData() const
{
    return m_tokenData;
}

void TokenModel::setTokenData(const QJsonObject &tokenData)
{
    if (m_tokenData != tokenData) {
        m_tokenData = tokenData;
        emit tokenDataChanged(m_tokenData);
    }
}

QDateTime TokenModel::expiresAt() const
{
    return m_expiresAt;
}

void TokenModel::setExpiresAt(const QDateTime &expiresAt)
{
    if (m_expiresAt != expiresAt) {
        m_expiresAt = expiresAt;
        emit expiresAtChanged(m_expiresAt);
    }
}

QDateTime TokenModel::createdAt() const
{
    return m_createdAt;
}

void TokenModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid TokenModel::createdBy() const
{
    return m_createdBy;
}

void TokenModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime TokenModel::updatedAt() const
{
    return m_updatedAt;
}

void TokenModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid TokenModel::updatedBy() const
{
    return m_updatedBy;
}

void TokenModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}

bool TokenModel::isRevoked() const
{
    return m_revoked;
}

void TokenModel::setRevoked(bool revoked)
{
    if (m_revoked != revoked) {
        m_revoked = revoked;
        emit revokedChanged(m_revoked);
    }
}

QString TokenModel::revocationReason() const
{
    return m_revocationReason;
}

void TokenModel::setRevocationReason(const QString &reason)
{
    if (m_revocationReason != reason) {
        m_revocationReason = reason;
        emit revocationReasonChanged(m_revocationReason);
    }
}

QJsonObject TokenModel::deviceInfo() const
{
    return m_deviceInfo;
}

void TokenModel::setDeviceInfo(const QJsonObject &deviceInfo)
{
    if (m_deviceInfo != deviceInfo) {
        m_deviceInfo = deviceInfo;
        emit deviceInfoChanged(m_deviceInfo);
    }
}

QDateTime TokenModel::lastUsedAt() const
{
    return m_lastUsedAt;
}

void TokenModel::setLastUsedAt(const QDateTime &lastUsedAt)
{
    if (m_lastUsedAt != lastUsedAt) {
        m_lastUsedAt = lastUsedAt;
        emit lastUsedAtChanged(m_lastUsedAt);
    }
}

bool TokenModel::isExpired() const
{
    return QDateTime::currentDateTimeUtc() > m_expiresAt;
}

bool TokenModel::isValid() const
{
    return !m_tokenId.isEmpty() && !isExpired() && !m_revoked;
}