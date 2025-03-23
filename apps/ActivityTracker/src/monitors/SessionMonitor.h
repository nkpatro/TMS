#ifndef SESSIONMONITOR_H
#define SESSIONMONITOR_H

#include <QObject>
#include <QString>

class SessionMonitor : public QObject
{
    Q_OBJECT
public:
    enum SessionState {
        Unknown = 0,
        Login = 1,
        Logout = 2,
        Lock = 3,
        Unlock = 4,
        SwitchUser = 5,
        RemoteConnect = 6,
        RemoteDisconnect = 7
    };
    Q_ENUM(SessionState)

    explicit SessionMonitor(QObject *parent = nullptr);
    virtual ~SessionMonitor();

    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual SessionState currentSessionState() const = 0;
    virtual QString currentUser() const = 0;
    virtual bool isRemoteSession() const = 0;

    signals:
        // Using int for the state to avoid enum class issues in signal/slot connections
        void sessionStateChanged(int newState, const QString &username);
    void afkStateChanged(bool isAfk);
};

#endif // SESSIONMONITOR_H