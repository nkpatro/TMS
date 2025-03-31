#include "UserRoleDisciplineModel.h"

UserRoleDisciplineModel::UserRoleDisciplineModel(QObject *parent)
    : QObject(parent)
{
    // m_id = QUuid::createUuid();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid UserRoleDisciplineModel::id() const
{
    return m_id;
}

void UserRoleDisciplineModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QUuid UserRoleDisciplineModel::userId() const
{
    return m_userId;
}

void UserRoleDisciplineModel::setUserId(const QUuid &userId)
{
    if (m_userId != userId) {
        m_userId = userId;
        emit userIdChanged(m_userId);
    }
}

QUuid UserRoleDisciplineModel::roleId() const
{
    return m_roleId;
}

void UserRoleDisciplineModel::setRoleId(const QUuid &roleId)
{
    if (m_roleId != roleId) {
        m_roleId = roleId;
        emit roleIdChanged(m_roleId);
    }
}

QUuid UserRoleDisciplineModel::disciplineId() const
{
    return m_disciplineId;
}

void UserRoleDisciplineModel::setDisciplineId(const QUuid &disciplineId)
{
    if (m_disciplineId != disciplineId) {
        m_disciplineId = disciplineId;
        emit disciplineIdChanged(m_disciplineId);
    }
}

QDateTime UserRoleDisciplineModel::createdAt() const
{
    return m_createdAt;
}

void UserRoleDisciplineModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid UserRoleDisciplineModel::createdBy() const
{
    return m_createdBy;
}

void UserRoleDisciplineModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime UserRoleDisciplineModel::updatedAt() const
{
    return m_updatedAt;
}

void UserRoleDisciplineModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid UserRoleDisciplineModel::updatedBy() const
{
    return m_updatedBy;
}

void UserRoleDisciplineModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}