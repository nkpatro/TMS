#ifndef ACTIVITYEVENTMODEL_H
#define ACTIVITYEVENTMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>
#include "EventTypes.h"

class ActivityEventModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QUuid sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(QUuid appId READ appId WRITE setAppId NOTIFY appIdChanged)
    Q_PROPERTY(EventTypes::ActivityEventType eventType READ eventType WRITE setEventType NOTIFY eventTypeChanged)
    Q_PROPERTY(QDateTime eventTime READ eventTime WRITE setEventTime NOTIFY eventTimeChanged)
    Q_PROPERTY(QJsonObject eventData READ eventData WRITE setEventData NOTIFY eventDataChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)

public:
    explicit ActivityEventModel(QObject *parent = nullptr);

    QUuid id() const;
    void setId(const QUuid &id);

    QUuid sessionId() const;
    void setSessionId(const QUuid &sessionId);

    QUuid appId() const;
    void setAppId(const QUuid &appId);

    EventTypes::ActivityEventType eventType() const;
    void setEventType(EventTypes::ActivityEventType eventType);

    QDateTime eventTime() const;
    void setEventTime(const QDateTime &eventTime);

    QJsonObject eventData() const;
    void setEventData(const QJsonObject &eventData);

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
    void sessionIdChanged(const QUuid &sessionId);
    void appIdChanged(const QUuid &appId);
    void eventTypeChanged(EventTypes::ActivityEventType eventType);
    void eventTimeChanged(const QDateTime &eventTime);
    void eventDataChanged(const QJsonObject &eventData);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);

private:
    QUuid m_id;
    QUuid m_sessionId;
    QUuid m_appId;
    EventTypes::ActivityEventType m_eventType;
    QDateTime m_eventTime;
    QJsonObject m_eventData;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;
};

#endif // ACTIVITYEVENTMODEL_H