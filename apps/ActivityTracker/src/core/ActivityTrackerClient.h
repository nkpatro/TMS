#ifndef ACTIVITYTRACKERCLIENT_H
#define ACTIVITYTRACKERCLIENT_H

#include <QObject>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>
#include <QTimer>
#include <QPoint>

// Forward declarations
class APIManager;
class SessionManager;
class MonitorManager;
class SessionStateMachine;
class SyncManager;
class ActivityMonitorBatcher;
class ConfigManager;

class ActivityTrackerClient : public QObject
{
    Q_OBJECT
public:
    explicit ActivityTrackerClient(QObject *parent = nullptr);
    ~ActivityTrackerClient();

    // Main lifecycle methods
    bool initialize(const QString &serverUrl, const QString &username, const QString &machineId);
    bool start();
    bool stop();
    bool isRunning() const;
    bool reload();

    // Configuration
    void setDataSendInterval(int milliseconds);
    void setIdleTimeThreshold(int milliseconds);

    // Properties
    QString serverUrl() const;
    QString username() const;
    QString machineId() const;
    QUuid sessionId() const;
    QDateTime lastSyncTime() const;
    bool isOfflineMode() const;

signals:
    // Status signals
    void statusChanged(const QString& status);
    void errorOccurred(const QString& errorMessage);
    void syncCompleted(bool success);
    void sessionStateChanged(const QString& state);

private slots:
    // Configuration changes
    void onConfigChanged();
    void onMachineIdChanged(const QString& machineId);

    // Batched monitor signals
    void onBatchedKeyboardActivity(int keyPressCount);
    void onBatchedMouseActivity(const QList<QPoint>& positions, int clickCount);
    void onBatchedAppActivity(const QString &appName, const QString &windowTitle,
                              const QString &executablePath, int focusChanges);

    // Session state machine signals
    void onSessionStateChanged(int newState, int oldState);

    // Sync manager signals
    void onConnectionStateChanged(bool online);
    void onSyncCompleted(bool success, int itemsProcessed);

    // System signals
    void onSystemMetricsUpdated(float cpuUsage, float gpuUsage, float ramUsage);
    void onHighCpuProcessDetected(const QString &processName, float cpuUsage);

    // Session monitor signals
    void onSessionStateChanged(int newState, const QString &username);
    void onAfkStateChanged(bool isAfk);

    // Day change handling
    void checkDayChange();

private:
    // Component managers
    APIManager* m_apiManager;
    SessionManager* m_sessionManager;
    MonitorManager* m_monitorManager;
    SessionStateMachine* m_sessionStateMachine;
    SyncManager* m_syncManager;
    ActivityMonitorBatcher* m_batcher;
    ConfigManager* m_configManager;

    // Properties
    QString m_serverUrl;
    QString m_username;
    QString m_machineId;
    bool m_isRunning;
    int m_idleTimeThreshold;
    int m_dataSendInterval;
    QDate m_currentSessionDay;

    // App tracking
    QString m_currentAppName;
    QString m_currentWindowTitle;
    QString m_currentAppPath;

    // Timers
    QTimer m_dayCheckTimer;

    // Helper methods
    bool handleDayChange();

    // Event recording methods
    bool recordSessionEvent(const QString &eventType, const QJsonObject &eventData = QJsonObject());
    bool recordActivityEvent(const QString &eventType, const QJsonObject &eventData = QJsonObject());
    bool recordSystemMetrics(float cpuUsage, float gpuUsage, float ramUsage);
};

#endif // ACTIVITYTRACKERCLIENT_H