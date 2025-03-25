// MultiUserManager.h
#ifndef MULTIUSERMANAGER_H
#define MULTIUSERMANAGER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QMap>

class MultiUserManager : public QObject
{
    Q_OBJECT
public:
    explicit MultiUserManager(QObject *parent = nullptr);
    ~MultiUserManager();

    bool initialize();
    bool start();
    bool stop();

    QString currentUser() const;
    QStringList activeUsers() const;

    signals:
        void userSessionChanged(const QString& username, bool active);

    private slots:
        void checkUserSessions();

private:
    QTimer m_pollTimer;
    bool m_isRunning;
    QString m_currentUser;
    QMap<QString, bool> m_activeUsers;
    QString m_previousUser;

    void updateActiveUsers(bool initialUpdate = false);
};

#endif // MULTIUSERMANAGER_H