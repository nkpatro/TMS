#ifndef APPLICATIONMODEL_H
#define APPLICATIONMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>

class ApplicationModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString appName READ appName WRITE setAppName NOTIFY appNameChanged)
    Q_PROPERTY(QString appPath READ appPath WRITE setAppPath NOTIFY appPathChanged)
    Q_PROPERTY(QString appHash READ appHash WRITE setAppHash NOTIFY appHashChanged)
    Q_PROPERTY(bool isRestricted READ isRestricted WRITE setIsRestricted NOTIFY isRestrictedChanged)
    Q_PROPERTY(bool trackingEnabled READ trackingEnabled WRITE setTrackingEnabled NOTIFY trackingEnabledChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)

public:
    explicit ApplicationModel(QObject *parent = nullptr);

    QUuid id() const;
    void setId(const QUuid &id);

    QString appName() const;
    void setAppName(const QString &appName);

    QString appPath() const;
    void setAppPath(const QString &appPath);

    QString appHash() const;
    void setAppHash(const QString &appHash);

    bool isRestricted() const;
    void setIsRestricted(bool isRestricted);

    bool trackingEnabled() const;
    void setTrackingEnabled(bool trackingEnabled);

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
    void appNameChanged(const QString &appName);
    void appPathChanged(const QString &appPath);
    void appHashChanged(const QString &appHash);
    void isRestrictedChanged(bool isRestricted);
    void trackingEnabledChanged(bool trackingEnabled);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);

private:
    QUuid m_id;
    QString m_appName;
    QString m_appPath;
    QString m_appHash;
    bool m_isRestricted;
    bool m_trackingEnabled;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;
};

#endif // APPLICATIONMODEL_H