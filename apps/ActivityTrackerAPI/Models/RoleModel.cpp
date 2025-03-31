#include "RoleModel.h"

RoleModel::RoleModel(QObject *parent)
    : QObject(parent)
{
    // m_id = QUuid::createUuid();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid RoleModel::id() const
{
    return m_id;
}

void RoleModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QString RoleModel::code() const
{
    return m_code;
}

void RoleModel::setCode(const QString &code)
{
    if (m_code != code) {
        m_code = code;
        emit codeChanged(m_code);
    }
}

QString RoleModel::name() const
{
    return m_name;
}

void RoleModel::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
    }
}

QString RoleModel::description() const
{
    return m_description;
}

void RoleModel::setDescription(const QString &description)
{
    if (m_description != description) {
        m_description = description;
        emit descriptionChanged(m_description);
    }
}

QDateTime RoleModel::createdAt() const
{
    return m_createdAt;
}

void RoleModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid RoleModel::createdBy() const
{
    return m_createdBy;
}

void RoleModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime RoleModel::updatedAt() const
{
    return m_updatedAt;
}

void RoleModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid RoleModel::updatedBy() const
{
    return m_updatedBy;
}

void RoleModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}