// SessionModel.h
#ifndef SESSIONMODEL_H
#define SESSIONMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>
#include <QHostAddress>
#include <QJsonObject>

class SessionModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QUuid userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QDateTime loginTime READ loginTime WRITE setLoginTime NOTIFY loginTimeChanged)
    Q_PROPERTY(QDateTime logoutTime READ logoutTime WRITE setLogoutTime NOTIFY logoutTimeChanged)
    Q_PROPERTY(QUuid machineId READ machineId WRITE setMachineId NOTIFY machineIdChanged)
    Q_PROPERTY(QJsonObject sessionData READ sessionData WRITE setSessionData NOTIFY sessionDataChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)

    // Session continuity properties
    Q_PROPERTY(QUuid continuedFromSession READ continuedFromSession WRITE setContinuedFromSession NOTIFY continuedFromSessionChanged)
    Q_PROPERTY(QUuid continuedBySession READ continuedBySession WRITE setContinuedBySession NOTIFY continuedBySessionChanged)
    Q_PROPERTY(QDateTime previousSessionEndTime READ previousSessionEndTime WRITE setPreviousSessionEndTime NOTIFY previousSessionEndTimeChanged)
    Q_PROPERTY(qint64 timeSincePreviousSession READ timeSincePreviousSession WRITE setTimeSincePreviousSession NOTIFY timeSincePreviousSessionChanged)

public:
    explicit SessionModel(QObject *parent = nullptr);
    QString debugInfo() const;

    QUuid id() const;
    void setId(const QUuid &id);

    QUuid userId() const;
    void setUserId(const QUuid &userId);

    QDateTime loginTime() const;
    void setLoginTime(const QDateTime &loginTime);

    QDateTime logoutTime() const;
    void setLogoutTime(const QDateTime &logoutTime);

    QUuid machineId() const;
    void setMachineId(const QUuid &machineId);

    QJsonObject sessionData() const;
    void setSessionData(const QJsonObject &sessionData);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime &createdAt);

    QUuid createdBy() const;
    void setCreatedBy(const QUuid &createdBy);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime &updatedAt);

    QUuid updatedBy() const;
    void setUpdatedBy(const QUuid &updatedBy);

    // Session continuity getters and setters
    QUuid continuedFromSession() const;
    void setContinuedFromSession(const QUuid &id);

    QUuid continuedBySession() const;
    void setContinuedBySession(const QUuid &id);

    QDateTime previousSessionEndTime() const;
    void setPreviousSessionEndTime(const QDateTime &endTime);

    qint64 timeSincePreviousSession() const;
    void setTimeSincePreviousSession(qint64 timeInSeconds);

    // Helper methods
    bool isActive() const;
    qint64 duration() const; // In seconds

signals:
    void idChanged(const QUuid &id);
    void userIdChanged(const QUuid &userId);
    void loginTimeChanged(const QDateTime &loginTime);
    void logoutTimeChanged(const QDateTime &logoutTime);
    void machineIdChanged(const QUuid &machineId);
    void sessionDataChanged(const QJsonObject &sessionData);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);
    void continuedFromSessionChanged(const QUuid &id);
    void continuedBySessionChanged(const QUuid &id);
    void previousSessionEndTimeChanged(const QDateTime &endTime);
    void timeSincePreviousSessionChanged(qint64 timeInSeconds);

private:
    QUuid m_id;
    QUuid m_userId;
    QDateTime m_loginTime;
    QDateTime m_logoutTime;
    QUuid m_machineId;
    QJsonObject m_sessionData;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;

    // Session continuity fields
    QUuid m_continuedFromSession;
    QUuid m_continuedBySession;
    QDateTime m_previousSessionEndTime;
    qint64 m_timeSincePreviousSession;
};

#endif // SESSIONMODEL_H