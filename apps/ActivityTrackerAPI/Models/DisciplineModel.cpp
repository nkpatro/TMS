// DisciplineModel.cpp
#include "DisciplineModel.h"

DisciplineModel::DisciplineModel(QObject *parent)
    : QObject(parent)
{
    // m_id = QUuid::createUuid();
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}

QUuid DisciplineModel::id() const
{
    return m_id;
}

void DisciplineModel::setId(const QUuid &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

QString DisciplineModel::code() const
{
    return m_code;
}

void DisciplineModel::setCode(const QString &code)
{
    if (m_code != code) {
        m_code = code;
        emit codeChanged(m_code);
    }
}

QString DisciplineModel::name() const
{
    return m_name;
}

void DisciplineModel::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
    }
}

QString DisciplineModel::description() const
{
    return m_description;
}

void DisciplineModel::setDescription(const QString &description)
{
    if (m_description != description) {
        m_description = description;
        emit descriptionChanged(m_description);
    }
}

QDateTime DisciplineModel::createdAt() const
{
    return m_createdAt;
}

void DisciplineModel::setCreatedAt(const QDateTime &createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged(m_createdAt);
    }
}

QUuid DisciplineModel::createdBy() const
{
    return m_createdBy;
}

void DisciplineModel::setCreatedBy(const QUuid &createdBy)
{
    if (m_createdBy != createdBy) {
        m_createdBy = createdBy;
        emit createdByChanged(m_createdBy);
    }
}

QDateTime DisciplineModel::updatedAt() const
{
    return m_updatedAt;
}

void DisciplineModel::setUpdatedAt(const QDateTime &updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged(m_updatedAt);
    }
}

QUuid DisciplineModel::updatedBy() const
{
    return m_updatedBy;
}

void DisciplineModel::setUpdatedBy(const QUuid &updatedBy)
{
    if (m_updatedBy != updatedBy) {
        m_updatedBy = updatedBy;
        emit updatedByChanged(m_updatedBy);
    }
}