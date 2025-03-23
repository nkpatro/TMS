#ifndef AFKPERIODMODEL_H
#define AFKPERIODMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>

class AfkPeriodModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QUuid sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(QDateTime startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(QDateTime endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)

public:
    explicit AfkPeriodModel(QObject *parent = nullptr);

    QUuid id() const;
    void setId(const QUuid &id);

    QUuid sessionId() const;
    void setSessionId(const QUuid &sessionId);

    QDateTime startTime() const;
    void setStartTime(const QDateTime &startTime);

    QDateTime endTime() const;
    void setEndTime(const QDateTime &endTime);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime &createdAt);

    QUuid createdBy() const;
    void setCreatedBy(const QUuid &createdBy);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime &updatedAt);

    QUuid updatedBy() const;
    void setUpdatedBy(const QUuid &updatedBy);

    // Helper methods
    bool isActive() const;
    qint64 duration() const; // In seconds

signals:
    void idChanged(const QUuid &id);
    void sessionIdChanged(const QUuid &sessionId);
    void startTimeChanged(const QDateTime &startTime);
    void endTimeChanged(const QDateTime &endTime);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);

private:
    QUuid m_id;
    QUuid m_sessionId;
    QDateTime m_startTime;
    QDateTime m_endTime;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;
};

#endif // AFKPERIODMODEL_H