#ifndef SESSIONEVENTMODEL_H
#define SESSIONEVENTMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>
#include "EventTypes.h"

class SessionEventModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QUuid sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(EventTypes::SessionEventType eventType READ eventType WRITE setEventType NOTIFY eventTypeChanged)
    Q_PROPERTY(QDateTime eventTime READ eventTime WRITE setEventTime NOTIFY eventTimeChanged)
    Q_PROPERTY(QUuid userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QUuid previousUserId READ previousUserId WRITE setPreviousUserId NOTIFY previousUserIdChanged)
    Q_PROPERTY(QUuid machineId READ machineId WRITE setMachineId NOTIFY machineIdChanged)
    Q_PROPERTY(QString terminalSessionId READ terminalSessionId WRITE setTerminalSessionId NOTIFY terminalSessionIdChanged)
    Q_PROPERTY(bool isRemote READ isRemote WRITE setIsRemote NOTIFY isRemoteChanged)
    Q_PROPERTY(QJsonObject eventData READ eventData WRITE setEventData NOTIFY eventDataChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)

public:
    explicit SessionEventModel(QObject *parent = nullptr);

    QUuid id() const;
    void setId(const QUuid &id);

    QUuid sessionId() const;
    void setSessionId(const QUuid &sessionId);

    EventTypes::SessionEventType eventType() const;
    void setEventType(EventTypes::SessionEventType eventType);

    QDateTime eventTime() const;
    void setEventTime(const QDateTime &eventTime);

    QUuid userId() const;
    void setUserId(const QUuid &userId);

    QUuid previousUserId() const;
    void setPreviousUserId(const QUuid &previousUserId);

    QUuid machineId() const;
    void setMachineId(const QUuid &machineId);

    QString terminalSessionId() const;
    void setTerminalSessionId(const QString &terminalSessionId);

    bool isRemote() const;
    void setIsRemote(bool isRemote);

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
    void eventTypeChanged(EventTypes::SessionEventType eventType);
    void eventTimeChanged(const QDateTime &eventTime);
    void userIdChanged(const QUuid &userId);
    void previousUserIdChanged(const QUuid &previousUserId);
    void machineIdChanged(const QUuid &machineId);
    void terminalSessionIdChanged(const QString &terminalSessionId);
    void isRemoteChanged(bool isRemote);
    void eventDataChanged(const QJsonObject &eventData);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);

private:
    QUuid m_id;
    QUuid m_sessionId;
    EventTypes::SessionEventType m_eventType;
    QDateTime m_eventTime;
    QUuid m_userId;
    QUuid m_previousUserId;
    QUuid m_machineId;
    QString m_terminalSessionId;
    bool m_isRemote;
    QJsonObject m_eventData;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;
};

#endif // SESSIONEVENTMODEL_H