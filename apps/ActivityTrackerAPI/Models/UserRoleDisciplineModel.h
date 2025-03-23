#ifndef USERROLEDISCIPLINEMODEL_H
#define USERROLEDISCIPLINEMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>

class UserRoleDisciplineModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QUuid userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QUuid roleId READ roleId WRITE setRoleId NOTIFY roleIdChanged)
    Q_PROPERTY(QUuid disciplineId READ disciplineId WRITE setDisciplineId NOTIFY disciplineIdChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)

public:
    explicit UserRoleDisciplineModel(QObject *parent = nullptr);

    QUuid id() const;
    void setId(const QUuid &id);

    QUuid userId() const;
    void setUserId(const QUuid &userId);

    QUuid roleId() const;
    void setRoleId(const QUuid &roleId);

    QUuid disciplineId() const;
    void setDisciplineId(const QUuid &disciplineId);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime &createdAt);

    QUuid createdBy() const;
    void setCreatedBy(const QUuid &createdBy);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime &updatedAt);

    QUuid updatedBy() const;
    void setUpdatedBy(const QUuid &updatedBy);

signals:
    void idChanged(const QUuid &id);
    void userIdChanged(const QUuid &userId);
    void roleIdChanged(const QUuid &roleId);
    void disciplineIdChanged(const QUuid &disciplineId);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);

private:
    QUuid m_id;
    QUuid m_userId;
    QUuid m_roleId;
    QUuid m_disciplineId;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;
};

#endif // USERROLEDISCIPLINEMODEL_H