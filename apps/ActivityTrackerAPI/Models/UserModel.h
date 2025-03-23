// UserModel.h
#ifndef USERMODEL_H
#define USERMODEL_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QDateTime>

class UserModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QString photo READ photo WRITE setPhoto NOTIFY photoChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool verified READ verified WRITE setVerified NOTIFY verifiedChanged)
    Q_PROPERTY(QString verificationCode READ verificationCode WRITE setVerificationCode NOTIFY verificationCodeChanged)
    Q_PROPERTY(QUuid statusId READ statusId WRITE setStatusId NOTIFY statusIdChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QUuid createdBy READ createdBy WRITE setCreatedBy NOTIFY createdByChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QUuid updatedBy READ updatedBy WRITE setUpdatedBy NOTIFY updatedByChanged)

public:
    explicit UserModel(QObject *parent = nullptr);

    QUuid id() const;
    void setId(const QUuid &id);

    QString name() const;
    void setName(const QString &name);

    QString email() const;
    void setEmail(const QString &email);

    QString password() const;
    void setPassword(const QString &password);

    QString photo() const;
    void setPhoto(const QString &photo);

    bool active() const;
    void setActive(bool active);

    bool verified() const;
    void setVerified(bool verified);

    QString verificationCode() const;
    void setVerificationCode(const QString &verificationCode);

    QUuid statusId() const;
    void setStatusId(const QUuid &statusId);

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
    void nameChanged(const QString &name);
    void emailChanged(const QString &email);
    void passwordChanged(const QString &password);
    void photoChanged(const QString &photo);
    void activeChanged(bool active);
    void verifiedChanged(bool verified);
    void verificationCodeChanged(const QString &verificationCode);
    void statusIdChanged(const QUuid &statusId);
    void createdAtChanged(const QDateTime &createdAt);
    void createdByChanged(const QUuid &createdBy);
    void updatedAtChanged(const QDateTime &updatedAt);
    void updatedByChanged(const QUuid &updatedBy);

private:
    QUuid m_id;
    QString m_name;
    QString m_email;
    QString m_password;
    QString m_photo;
    bool m_active;
    bool m_verified;
    QString m_verificationCode;
    QUuid m_statusId;
    QDateTime m_createdAt;
    QUuid m_createdBy;
    QDateTime m_updatedAt;
    QUuid m_updatedBy;
};

#endif // USERMODEL_H