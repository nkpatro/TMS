#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QDateTime>

class APIManager;
class SessionManager;

class SyncManager : public QObject
{
    Q_OBJECT
public:
    // Data types for queue entries - using enum instead of enum class for compatibility
    enum DataType {
        SessionEvent = 0,
        ActivityEvent = 1,
        AppUsage = 2,
        SystemMetrics = 3,
        AfkPeriod = 4
    };
    
    struct QueuedData {
        DataType type;
        QUuid sessionId;
        QJsonObject data;
        QDateTime timestamp;
        int retryCount;
    };
    
    explicit SyncManager(APIManager* apiManager, SessionManager* sessionManager, QObject *parent = nullptr);
    ~SyncManager();
    
    bool initialize(int syncIntervalMs = 60000, int maxQueueSize = 1000);
    bool start();
    bool stop();
    
    // Session operations
    bool createOrReopenSession(const QDate& date, QUuid& sessionId, QDateTime& sessionStart, bool& isNewSession);
    bool closeSession(const QUuid& sessionId);
    
    // Queue management
    bool queueData(DataType type, const QUuid& sessionId, const QJsonObject& data, 
                  const QDateTime& timestamp = QDateTime::currentDateTime());
    bool processPendingQueue(int maxItems = 50);
    bool isOfflineMode() const { return m_offlineMode; }
    int queueSize() const { return m_dataQueue.size(); }
    
public slots:
    void checkConnection();
    void forceSyncNow();
    
signals:
    void connectionStateChanged(bool online);
    void syncCompleted(bool success, int itemsProcessed);
    void queueSizeChanged(int newSize);
    void dataProcessed(DataType type, const QUuid& sessionId, bool success);
    
private slots:
    void onSyncTimerTriggered();
    
private:
    APIManager* m_apiManager;
    SessionManager* m_sessionManager;
    QTimer m_syncTimer;
    QTimer m_connectionCheckTimer;
    QQueue<QueuedData> m_dataQueue;
    QMutex m_queueMutex;
    bool m_offlineMode;
    bool m_isRunning;
    bool m_initialized;
    int m_maxQueueSize;
    int m_syncInterval;
    QDateTime m_lastSyncTime;
    QDateTime m_lastConnectionCheck;
    
    bool batchAndProcessData();
    bool sendBatchedData(const QUuid& sessionId, 
                        const QJsonArray& sessionEvents,
                        const QJsonArray& activityEvents,
                        const QJsonArray& systemMetrics);
    
    // Helper methods
    bool registerMachine(const QString& hostname, QString& machineId);
    bool authenticateUser(const QString& username, const QString& machineId);
};

#endif // SYNCMANAGER_H