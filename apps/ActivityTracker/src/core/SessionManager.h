// Fixed SessionManager.h - Corrected parameter passing
#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QQueue>
#include <QMap>
#include "APIManager.h"

class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager();

    bool initialize(APIManager *apiManager, const QString &username, const QString &machineId);

    // Session management
    bool createOrReopenSession(const QDate &date, QUuid &sessionId, QDateTime &sessionStart, bool &isNewSession);
    bool closeSession(const QUuid &sessionId);

    // Event recording
    bool recordSessionEvent(const QUuid &sessionId, const QString &eventType, const QJsonObject &eventData = QJsonObject());
    bool recordLoginEvent(const QUuid &sessionId, const QDateTime &loginTime, const QJsonObject &eventData = QJsonObject());
    bool recordMissingLogoutEvent(const QUuid &sessionId, const QDateTime &logoutTime, const QJsonObject &eventData = QJsonObject());
    bool recordActivityEvent(const QUuid &sessionId, const QString &eventType, const QJsonObject &eventData = QJsonObject());

    // App usage
    bool startAppUsage(const QUuid &sessionId, const QString &appName, const QString &windowTitle,
                       const QString &executablePath, QUuid &usageId);
    bool endAppUsage(const QUuid &usageId);
    bool getApplicationUsages(const QUuid &sessionId, bool activeOnly, QJsonObject &usageData);
    bool getTopApplications(const QUuid &sessionId, int limit, QJsonObject &topAppsData);
    bool getActiveApplications(const QUuid &sessionId, QJsonObject &activeAppsData);
    bool detectOrCreateApplication(const QString &appName, const QString &appPath, const QString &appHash,
                                 bool isRestricted, bool trackingEnabled, QJsonObject &appData);

    // System metrics
    bool recordSystemMetrics(const QUuid &sessionId, float cpuUsage, float gpuUsage, float ramUsage);
    bool getSystemMetricsAverage(const QUuid &sessionId, QJsonObject &metricsData);
    bool getSystemMetricsTimeSeries(const QUuid &sessionId, const QString &metricType, QJsonObject &timeSeriesData);

    // AFK periods
    bool startAfkPeriod(const QUuid &sessionId);
    bool endAfkPeriod(const QUuid &sessionId);
    bool getAllAfkPeriods(const QUuid &sessionId, QJsonObject &afkData);

    // Data query
    bool getLastSessionLogoutTime(const QUuid &sessionId, QDateTime &logoutTime);
    bool getLastSessionLockTime(const QUuid &sessionId, QDateTime &lockTime);
    bool getLastEventTime(const QUuid &sessionId, QDateTime &eventTime);
    bool getSessionStatistics(const QUuid &sessionId, QJsonObject &statsData);
    bool getSessionChain(const QUuid &sessionId, QJsonObject &chainData);

    // Machine management
    bool registerMachine(const QString &name, const QString &operatingSystem, const QString &machineUniqueId = QString(),
                       const QString &macAddress = QString(), const QString &cpuInfo = QString(),
                       const QString &gpuInfo = QString(), int ramSizeGB = 0,
                       const QString &lastKnownIp = QString(), QJsonObject *machineData = nullptr);
    bool updateMachineStatus(const QString &machineId, bool active);
    bool updateMachineLastSeen(const QString &machineId);

    // Batch operations
    bool processBatchData(const QUuid &sessionId, const QJsonArray &activityEvents, const QJsonArray &appUsages,
                        const QJsonArray &systemMetrics, const QJsonArray &sessionEvents, QJsonObject &responseData);

    // Server connection
    bool checkServerConnection(bool &isConnected);

    // Data synchronization
    bool syncPendingData();

    QString getUsername() const { return m_username; }
    QString getMachineId() const { return m_machineId; }
    void updateMachineId(const QString &machineId) { m_machineId = machineId; }

private:
    enum class DataType {
        SessionEvent,
        ActivityEvent,
        AppUsage,
        SystemMetrics,
        AfkPeriod
    };

    struct PendingData {
        DataType type;
        QUuid sessionId;
        QJsonObject data;
        QDateTime timestamp;
    };

    bool addToPendingQueue(DataType type, const QUuid &sessionId, const QJsonObject &data,
                          const QDateTime &timestamp = QDateTime());
    bool processQueue(int maxItems = 50);

    APIManager *m_apiManager;
    QString m_username;
    QString m_machineId;
    QUuid m_activeAfkPeriodId;

    // Local cache for active sessions and app usages
    QMap<QUuid, QDateTime> m_sessionStarts;
    QMap<QUuid, QUuid> m_appUsageIds;

    // Queue for pending data to be sent to the server
    QQueue<PendingData> m_pendingQueue;

    // Local cache of session events for validation
    QMap<QUuid, QDateTime> m_lastSessionLogoutTimes;
    QMap<QUuid, QDateTime> m_lastSessionLockTimes;
    QMap<QUuid, QDateTime> m_lastEventTimes;

    // Local cache of successfully created sessions
    QMap<QDate, QUuid> m_sessionsByDate;

    // Maximum queue size before forcing a sync
    int m_maxQueueSize;
    bool m_initialized;
};

#endif // SESSIONMANAGER_H