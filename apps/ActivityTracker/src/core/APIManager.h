// APIManager.h
#ifndef APIMANAGER_H
#define APIMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QDate>
#include <QMutex>

class APIManager : public QObject
{
    Q_OBJECT
public:
    explicit APIManager(QObject *parent = nullptr);
    ~APIManager();

    bool initialize(const QString &serverUrl);

    // Authentication
    bool authenticate(const QString &username, const QString &machineId, QJsonObject &responseData);
    bool isAuthenticated() const;
    bool logout(QJsonObject &responseData);
    bool getUserProfile(QJsonObject &responseData);
    bool refreshToken(const QString &refreshToken, QJsonObject &responseData);

    // Session management
    bool findSessionForDate(const QJsonObject &query, QJsonObject &responseData);
    bool createSession(const QJsonObject &sessionData, QJsonObject &responseData);
    bool getSession(const QUuid &sessionId, QJsonObject &responseData);
    bool endSession(const QUuid &sessionId, QJsonObject &responseData);
    bool getAllSessions(bool activeOnly, QJsonObject &responseData);
    bool getActiveSession(const QString &machineId, QJsonObject &responseData);
    bool getSessionsByUser(const QString &userId, bool activeOnly, QJsonObject &responseData);
    bool getSessionsByMachine(const QString &machineId, bool activeOnly, QJsonObject &responseData);
    bool getSessionStats(const QUuid &sessionId, QJsonObject &responseData);
    bool getUserStats(const QString &userId, const QDate &startDate, const QDate &endDate, QJsonObject &responseData);
    bool getSessionChain(const QUuid &sessionId, QJsonObject &responseData);

    // AFK periods
    bool startAfkPeriod(const QJsonObject &afkData, QJsonObject &responseData);
    bool endAfkPeriod(const QUuid &afkId, const QJsonObject &afkData, QJsonObject &responseData);
    bool getAfkPeriods(const QUuid &sessionId, QJsonObject &responseData);

    // Event management
    bool getLastSessionEvent(const QJsonObject &query, QJsonObject &responseData);
    bool getLastEvent(const QJsonObject &query, QJsonObject &responseData);
    bool batchSessionEvents(const QJsonObject &eventsData);
    bool batchActivityEvents(const QJsonObject &eventsData);
    bool getSessionEvents(const QUuid &sessionId, int limit, int offset, QJsonObject &responseData);
    bool getSessionEventsByType(const QUuid &sessionId, const QString &eventType, int limit, int offset, QJsonObject &responseData);
    bool getSessionActivities(const QUuid &sessionId, int limit, int offset, QJsonObject &responseData);
    bool getSessionActivitiesByType(const QUuid &sessionId, const QString &eventType, int limit, int offset, QJsonObject &responseData);
    bool getSessionActivitiesByTimeRange(const QUuid &sessionId, const QDateTime &startTime, const QDateTime &endTime, int limit, int offset, QJsonObject &responseData);
    bool createSessionEvent(const QUuid &sessionId, const QJsonObject &eventData, QJsonObject &responseData);
    bool createActivityEvent(const QUuid &sessionId, const QJsonObject &eventData, QJsonObject &responseData);

    // App usage
    bool startAppUsage(const QJsonObject &usageData, QJsonObject &responseData);
    bool endAppUsage(const QUuid &usageId, const QJsonObject &usageData, QJsonObject &responseData);
    bool getAppUsages(const QUuid &sessionId, bool activeOnly, QJsonObject &responseData);
    bool getAppUsagesByApp(const QString &appId, int limit, QJsonObject &responseData);
    bool getAppUsageStats(const QUuid &sessionId, QJsonObject &responseData);
    bool getTopApps(const QUuid &sessionId, int limit, QJsonObject &responseData);
    bool getActiveApps(const QUuid &sessionId, QJsonObject &responseData);

    // Application management
    bool getAllApplications(QJsonObject &responseData);
    bool getApplication(const QString &appId, QJsonObject &responseData);
    bool createApplication(const QJsonObject &appData, QJsonObject &responseData);
    bool updateApplication(const QString &appId, const QJsonObject &appData, QJsonObject &responseData);
    bool deleteApplication(const QString &appId);
    bool getRestrictedApplications(QJsonObject &responseData);
    bool getTrackedApplications(QJsonObject &responseData);
    bool detectApplication(const QJsonObject &appData, QJsonObject &responseData);

    // System metrics
    bool batchSystemMetrics(const QJsonObject &metricsData);
    bool getSystemMetrics(const QUuid &sessionId, int limit, int offset, QJsonObject &responseData);
    bool recordSystemMetrics(const QUuid &sessionId, const QJsonObject &metricsData, QJsonObject &responseData);
    bool getAverageMetrics(const QUuid &sessionId, QJsonObject &responseData);
    bool getMetricsTimeSeries(const QUuid &sessionId, const QString &metricType, QJsonObject &responseData);
    bool getCurrentSystemInfo(QJsonObject &responseData);

    // Machine management
    bool getAllMachines(QJsonObject &responseData);
    bool getActiveMachines(QJsonObject &responseData);
    bool getMachineByName(const QString &name, QJsonObject &responseData);
    bool getMachine(const QString &machineId, QJsonObject &responseData);
    bool createMachine(const QJsonObject &machineData, QJsonObject &responseData);
    bool registerMachine(const QJsonObject &machineData, QJsonObject &responseData);
    bool updateMachine(const QString &machineId, const QJsonObject &machineData, QJsonObject &responseData);
    bool updateMachineStatus(const QString &machineId, bool active, QJsonObject &responseData);
    bool updateMachineLastSeen(const QString &machineId, const QDateTime &timestamp, QJsonObject &responseData);
    bool deleteMachine(const QString &machineId, QJsonObject &responseData);

    // Batch operations
    bool processBatch(const QJsonObject &batchData, QJsonObject &responseData);
    bool processSessionBatch(const QUuid &sessionId, const QJsonObject &batchData, QJsonObject &responseData);

    // Server status
    bool ping(QJsonObject &responseData);
    bool getServerHealth(QJsonObject &responseData);
    bool getServerVersion(QJsonObject &responseData);
    bool getServerConfiguration(QJsonObject& configData);

    int getLastErrorCode() const { return m_lastErrorCode; }
    QString getLastErrorMessage() const { return m_lastErrorMessage; }

    QString getAuthToken();
    bool setAuthToken(const QString& token);

private:
    bool sendRequest(const QString &endpoint, const QJsonObject &data, QJsonObject &responseData,
                    const QString &method = "POST", bool requiresAuth = true);
    bool processReply(QNetworkReply *reply, QJsonObject &responseData);

    QNetworkAccessManager *m_networkManager;
    QString m_serverUrl;
    QString m_authToken;
    QMutex m_mutex; // For thread safety
    bool m_initialized;
    QString m_username;
    QString m_machineId;
    int m_lastErrorCode = 0;
    QString m_lastErrorMessage;

};

#endif // APIMANAGER_H