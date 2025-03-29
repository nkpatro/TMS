// MultiUserManager.h
#ifndef MULTIUSERMANAGER_H
#define MULTIUSERMANAGER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QMap>
#include <QDateTime>

class APIManager;

// Enum to track user session state
enum class UserSessionState {
    Active,
    Locked,
    Inactive,
    LoggedOut
};

class MultiUserManager : public QObject
{
    Q_OBJECT
public:
    explicit MultiUserManager(QObject *parent = nullptr);
    ~MultiUserManager();

    bool initialize();
    bool start();
    bool stop();

    // Current user accessors
    QString currentUser() const;
    QStringList activeUsers() const;

    // New methods for enhanced user management
    UserSessionState getUserState(const QString& username) const;
    bool setUserState(const QString& username, UserSessionState state);
    QDateTime getLastActivityTime(const QString& username) const;
    void updateUserActivity(const QString& username);
    bool hasUserAuthToken(const QString& username) const;
    QString getUserAuthToken(const QString& username) const;
    bool setUserAuthToken(const QString& username, const QString& token);
    bool switchToUser(const QString& username);
    bool pauseCurrentUser();
    bool resumeUser(const QString& username);
    bool authenticateUser(const QString& username, const QString& machineId, APIManager* apiManager);

signals:
    void userSessionChanged(const QString& username, bool active);
    // New signals
    void userStateChanged(const QString& username, UserSessionState newState);
    void currentUserChanged(const QString& previousUser, const QString& newUser);
    void userTokenChanged(const QString& username, bool hasToken);

private slots:
    void checkUserSessions();

private:
    QTimer m_pollTimer;
    bool m_isRunning;
    QString m_currentUser;
    QString m_previousUser;
    QMap<QString, bool> m_activeUsers;

    // New storage for additional user information
    QMap<QString, UserSessionState> m_userStates;
    QMap<QString, QString> m_userAuthTokens;
    QMap<QString, QDateTime> m_userLastActivity;

    void updateActiveUsers(bool initialUpdate = false);
    void processUserStateChange(const QString& username, UserSessionState newState);
};

#endif // MULTIUSERMANAGER_H