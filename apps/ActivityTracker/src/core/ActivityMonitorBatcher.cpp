#include "ActivityMonitorBatcher.h"
#include "logger/logger.h"

ActivityMonitorBatcher::ActivityMonitorBatcher(QObject *parent)
    : QObject(parent)
    , m_isRunning(false)
    , m_mouseClickCount(0)
    , m_keyPressCount(0)
    , m_appFocusChanges(0)
    , m_appDataChanged(false)
{
    connect(&m_batchTimer, &QTimer::timeout, this, &ActivityMonitorBatcher::processBatch);
}

ActivityMonitorBatcher::~ActivityMonitorBatcher()
{
    if (m_isRunning) {
        stop();
    }
}

void ActivityMonitorBatcher::initialize(int batchIntervalMs)
{
    LOG_INFO(QString("Initializing ActivityMonitorBatcher with interval: %1ms").arg(batchIntervalMs));

    // Set up the batch timer
    m_batchTimer.setInterval(batchIntervalMs);

    // If interval is 0, don't batch - immediately process events
    if (batchIntervalMs <= 0) {
        disconnect(&m_batchTimer, &QTimer::timeout, this, &ActivityMonitorBatcher::processBatch);
    }
}

void ActivityMonitorBatcher::start()
{
    if (m_isRunning) {
        LOG_WARNING("ActivityMonitorBatcher already running");
        return;
    }

    LOG_INFO("Starting ActivityMonitorBatcher");

    // Only start timer if interval > 0
    if (m_batchTimer.interval() > 0) {
        m_batchTimer.start();
    }

    m_isRunning = true;
}

void ActivityMonitorBatcher::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("ActivityMonitorBatcher not running");
        return;
    }

    LOG_INFO("Stopping ActivityMonitorBatcher");

    m_batchTimer.stop();

    // Process any remaining events
    processBatch();

    m_isRunning = false;
}

void ActivityMonitorBatcher::addMouseEvent(int x, int y, bool clicked)
{
    QMutexLocker locker(&m_mutex);

    // Add mouse position
    m_mousePositions.append(QPoint(x, y));

    // Count clicks
    if (clicked) {
        m_mouseClickCount++;
    }

    // If not batching (interval <= 0), process immediately
    if (m_batchTimer.interval() <= 0) {
        locker.unlock();
        processBatch();
    }
}

void ActivityMonitorBatcher::addKeyboardEvent()
{
    QMutexLocker locker(&m_mutex);

    // Increment key press count
    m_keyPressCount++;

    // If not batching (interval <= 0), process immediately
    if (m_batchTimer.interval() <= 0) {
        locker.unlock();
        processBatch();
    }
}

void ActivityMonitorBatcher::addAppEvent(const QString &appName, const QString &windowTitle, const QString &executablePath)
{
    QMutexLocker locker(&m_mutex);

    // Check if app has changed
    if (m_currentAppName != appName || m_currentWindowTitle != windowTitle || m_currentAppPath != executablePath) {
        m_currentAppName = appName;
        m_currentWindowTitle = windowTitle;
        m_currentAppPath = executablePath;
        m_appFocusChanges++;
        m_appDataChanged = true;
    }

    // If not batching (interval <= 0), process immediately
    if (m_batchTimer.interval() <= 0) {
        locker.unlock();
        processBatch();
    }
}

void ActivityMonitorBatcher::processBatch()
{
    QMutexLocker locker(&m_mutex);

    // Process mouse events if there are any
    if (!m_mousePositions.isEmpty() || m_mouseClickCount > 0) {
        QList<QPoint> positions = m_mousePositions;
        int clickCount = m_mouseClickCount;

        m_mousePositions.clear();
        m_mouseClickCount = 0;

        locker.unlock();
        emit batchedMouseActivity(positions, clickCount);
        locker.relock();
    }

    // Process keyboard events if there are any
    if (m_keyPressCount > 0) {
        int keyPressCount = m_keyPressCount;
        m_keyPressCount = 0;

        locker.unlock();
        emit batchedKeyboardActivity(keyPressCount);
        locker.relock();
    }

    // Process app events if there are any
    if (m_appDataChanged) {
        QString appName = m_currentAppName;
        QString windowTitle = m_currentWindowTitle;
        QString appPath = m_currentAppPath;
        int focusChanges = m_appFocusChanges;

        m_appFocusChanges = 0;
        m_appDataChanged = false;

        locker.unlock();
        emit batchedAppActivity(appName, windowTitle, appPath, focusChanges);
    }
}