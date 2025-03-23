#ifndef APPUSAGEMODEL_H
#define APPUSAGEMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>

class AppUsageModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QUuid sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(QUuid appId READ appId WRITE setAppId NOTIFY appIdChanged)
    Q_PROPERTY(QDateTime startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(QDateTime endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged)
    Q_PROPERTY(bool isActive READ isActive WRITE setIsActive NOTIFY isActiveChanged)
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle NOTIFY windowTitleChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)

public:
    explicit AppUsageModel(QObject *parent = nullptr);

    QUuid id() const;
    void setId(const QUuid &id);

    QUuid sessionId() const;
    void setSessionId(const QUuid &sessionId);

    QUuid appId() const;
    void setAppId(const QUuid &appId);

    QDateTime startTime() const;
    void setStartTime(const QDateTime &startTime);

    QDateTime endTime() const;
    void setEndTime(const QDateTime &endTime);

    bool isActive() const;
    void setIsActive(bool isActive);

    QString windowTitle() const;
    void setWindowTitle(const QString &windowTitle);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime &createdAt);

    QUuid createdBy() const;
    void setCreatedBy(const QUuid &createdBy);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime &updatedAt);

    QUuid updatedBy() const;
    void setUpdatedBy(const QUuid &updatedBy);

    // Helper method
    qint64 duration() const; // In seconds

signals:
    void idChanged(const QUuid &id);
    void sessionIdChanged(const QUuid &sessionId);
    void appIdChanged(const QUuid &appId);
    void startTimeChanged(const QDateTime &startTime);
    void endTimeChanged(const QDateTime &endTime);
    void isActiveChanged(bool isActive);
    void windowTitleChanged(const QString &windowTitle);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);

private:
    QUuid m_id;
    QUuid m_sessionId;
    QUuid m_appId;
    QDateTime m_startTime;
    QDateTime m_endTime;
    bool m_isActive;
    QString m_windowTitle;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;
};

#endif // APPUSAGEMODEL_H