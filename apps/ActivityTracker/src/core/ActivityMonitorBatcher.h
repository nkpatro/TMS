#ifndef ACTIVITYMONITORBATCHER_H
#define ACTIVITYMONITORBATCHER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QPoint>
#include <QList>

class ActivityMonitorBatcher : public QObject
{
    Q_OBJECT
public:
    explicit ActivityMonitorBatcher(QObject *parent = nullptr);
    ~ActivityMonitorBatcher();

    void initialize(int batchIntervalMs = 100);
    void start();
    void stop();

    public slots:
        // Add events to the batch
        void addMouseEvent(int x, int y, bool clicked);
    void addKeyboardEvent();
    void addAppEvent(const QString &appName, const QString &windowTitle, const QString &executablePath);

    signals:
        // Batched events
        void batchedMouseActivity(const QList<QPoint>& positions, int clickCount);
    void batchedKeyboardActivity(int keyPressCount);
    void batchedAppActivity(const QString &appName, const QString &windowTitle,
                           const QString &executablePath, int focusChanges);

    private slots:
        void processBatch();

private:
    QTimer m_batchTimer;
    QMutex m_mutex;
    bool m_isRunning;

    // Batched event data
    QList<QPoint> m_mousePositions;
    int m_mouseClickCount;
    int m_keyPressCount;

    // App focus data
    QString m_currentAppName;
    QString m_currentWindowTitle;
    QString m_currentAppPath;
    int m_appFocusChanges;

    bool m_appDataChanged;
};

#endif // ACTIVITYMONITORBATCHER_H