// UserModel.cpp
#include "UserModel.h"

UserModel::UserModel(QObject *parent)
    : QObject(parent),
      m_active(true),
      m_verified(false)
{
    // m_id = QUuid::createUuid();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid UserModel::id() const
{
    return m_id;
}

void UserModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QString UserModel::name() const
{
    return m_name;
}

void UserModel::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
    }
}

QString UserModel::email() const
{
    return m_email;
}

void UserModel::setEmail(const QString &email)
{
    if (m_email != email) {
        m_email = email;
        emit emailChanged(m_email);
    }
}

QString UserModel::password() const
{
    return m_password;
}

void UserModel::setPassword(const QString &password)
{
    if (m_password != password) {
        m_password = password;
        emit passwordChanged(m_password);
    }
}

QString UserModel::photo() const
{
    return m_photo;
}

void UserModel::setPhoto(const QString &photo)
{
    if (m_photo != photo) {
        m_photo = photo;
        emit photoChanged(m_photo);
    }
}

bool UserModel::active() const
{
    return m_active;
}

void UserModel::setActive(bool active)
{
    if (m_active != active) {
        m_active = active;
        emit activeChanged(m_active);
    }
}

bool UserModel::verified() const
{
    return m_verified;
}

void UserModel::setVerified(bool verified)
{
    if (m_verified != verified) {
        m_verified = verified;
        emit verifiedChanged(m_verified);
    }
}

QString UserModel::verificationCode() const
{
    return m_verificationCode;
}

void UserModel::setVerificationCode(const QString &verificationCode)
{
    if (m_verificationCode != verificationCode) {
        m_verificationCode = verificationCode;
        emit verificationCodeChanged(m_verificationCode);
    }
}

QUuid UserModel::statusId() const
{
    return m_statusId;
}

void UserModel::setStatusId(const QUuid &statusId)
{
    if (m_statusId != statusId) {
        m_statusId = statusId;
        emit statusIdChanged(m_statusId);
    }
}

QDateTime UserModel::createdAt() const
{
    return m_createdAt;
}

void UserModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid UserModel::createdBy() const
{
    return m_createdBy;
}

void UserModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime UserModel::updatedAt() const
{
    return m_updatedAt;
}

void UserModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid UserModel::updatedBy() const
{
    return m_updatedBy;
}

void UserModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}