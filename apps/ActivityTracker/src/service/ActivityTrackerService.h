#ifndef ACTIVITYTRACKERSERVICE_H
#define ACTIVITYTRACKERSERVICE_H

#include <QObject>
#include <QTimer>
#include "../core/ActivityTrackerClient.h"
#include "../managers/ConfigManager.h"
#include "MultiUserManager.h"

class ActivityTrackerService : public QObject
{
    Q_OBJECT
public:
    explicit ActivityTrackerService(QObject *parent = nullptr);
    ~ActivityTrackerService();

    bool initialize();
    bool start();
    bool stop();
    bool reload();
    bool isRunning() const;

    public slots:
        void onConfigChanged();
    void onUserSessionChanged(const QString& username, bool active);
    void onStatusChanged(const QString& status);
    void onErrorOccurred(const QString& errorMessage);

private:
    bool loadConfig();
    void setupSignalHandlers();
    bool authenticateCurrentUser();

    ActivityTrackerClient* m_trackerClient;
    MultiUserManager* m_userManager;
    ConfigManager* m_configManager;
    QTimer m_heartbeatTimer;
    bool m_isRunning;
    QString m_currentUser;

    // Settings
    QString m_serverUrl;
    int m_dataSendInterval;
    int m_idleTimeThreshold;
    bool m_trackKeyboardMouse;
    bool m_trackApplications;
    bool m_trackSystemMetrics;
    bool m_multiUserMode;
    QString m_machineId;
};

#endif // ACTIVITYTRACKERSERVICE_H