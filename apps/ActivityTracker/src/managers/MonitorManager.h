#ifndef MONITORMANAGER_H
#define MONITORMANAGER_H

#include <QObject>
#include <QVector>
#include "../monitors/KeyboardMouseMonitor.h"
#include "../monitors/AppMonitor.h"
#include "../monitors/SessionMonitor.h"
#include "../monitors/SystemMonitor.h"

class ActivityMonitorBatcher;

class MonitorManager : public QObject
{
    Q_OBJECT
public:
    explicit MonitorManager(QObject *parent = nullptr);
    ~MonitorManager();

    bool initialize(bool trackKeyboardMouse, bool trackApplications, bool trackSystemMetrics);
    bool start();
    bool stop();
    bool isRunning() const;

    // Connect signals to target object
    void connectMonitorSignals(ActivityMonitorBatcher* batcher);

    // Access to monitors
    KeyboardMouseMonitor* keyboardMouseMonitor() const { return m_keyboardMouseMonitor; }
    AppMonitor* appMonitor() const { return m_appMonitor; }
    SessionMonitor* sessionMonitor() const { return m_sessionMonitor; }
    SystemMonitor* systemMonitor() const { return m_systemMonitor; }

    // Configuration
    void setIdleTimeThreshold(int milliseconds);
    void setHighCpuThreshold(float percentage);

    signals:
        // Pass-through signals from monitors to client
        void systemMetricsUpdated(float cpuUsage, float gpuUsage, float ramUsage);
    void highCpuProcessDetected(const QString &processName, float cpuUsage);
//    void sessionStateChanged(SessionMonitor::SessionState newState, const QString &username);
    void sessionStateChanged(int newState, const QString &username);
    void afkStateChanged(bool isAfk);

private:
    KeyboardMouseMonitor* m_keyboardMouseMonitor;
    AppMonitor* m_appMonitor;
    SessionMonitor* m_sessionMonitor;
    SystemMonitor* m_systemMonitor;

    bool m_isRunning;
    bool m_trackKeyboardMouse;
    bool m_trackApplications;
    bool m_trackSystemMetrics;

    // Create platform-specific monitors
    void createPlatformMonitors();
};

#endif // MONITORMANAGER_H