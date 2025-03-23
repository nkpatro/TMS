#include "MonitorManager.h"
#include "logger/logger.h"
#include "../core/ActivityMonitorBatcher.h"

// Include platform-specific monitor implementations based on compiler defines
#ifdef Q_OS_WIN
#include "../monitors/win/KeyboardMouseMonitorWin.h"
#include "../monitors/win/AppMonitorWin.h"
#include "../monitors/win/SessionMonitorWin.h"
#include "../monitors/win/SystemMonitorWin.h"
#elif defined(Q_OS_MACOS)
#include "../monitors/mac/KeyboardMouseMonitorMac.h"
#include "../monitors/mac/AppMonitorMac.h"
#include "../monitors/mac/SessionMonitorMac.h"
#include "../monitors/mac/SystemMonitorMac.h"
#elif defined(Q_OS_LINUX)
#include "../monitors/linux/KeyboardMouseMonitorLinux.h"
#include "../monitors/linux/AppMonitorLinux.h"
#include "../monitors/linux/SessionMonitorLinux.h"
#include "../monitors/linux/SystemMonitorLinux.h"
#endif

MonitorManager::MonitorManager(QObject *parent)
    : QObject(parent)
    , m_keyboardMouseMonitor(nullptr)
    , m_appMonitor(nullptr)
    , m_sessionMonitor(nullptr)
    , m_systemMonitor(nullptr)
    , m_isRunning(false)
    , m_trackKeyboardMouse(true)
    , m_trackApplications(true)
    , m_trackSystemMetrics(true)
{
}

MonitorManager::~MonitorManager()
{
    if (m_isRunning) {
        stop();
    }
}

bool MonitorManager::initialize(bool trackKeyboardMouse, bool trackApplications, bool trackSystemMetrics)
{
    LOG_INFO("Initializing MonitorManager");

    m_trackKeyboardMouse = trackKeyboardMouse;
    m_trackApplications = trackApplications;
    m_trackSystemMetrics = trackSystemMetrics;

    // Create platform-specific monitors
    createPlatformMonitors();

    // Initialize monitors according to tracking settings
    bool initSuccess = true;

    if (m_trackKeyboardMouse && m_keyboardMouseMonitor) {
        if (!m_keyboardMouseMonitor->initialize()) {
            LOG_ERROR("Failed to initialize keyboard/mouse monitor");
            initSuccess = false;
        }
    }

    if (m_trackApplications && m_appMonitor) {
        if (!m_appMonitor->initialize()) {
            LOG_ERROR("Failed to initialize app monitor");
            initSuccess = false;
        }
    }

    // Session monitor is always required
    if (m_sessionMonitor) {
        if (!m_sessionMonitor->initialize()) {
            LOG_ERROR("Failed to initialize session monitor");
            initSuccess = false;
        }
    } else {
        LOG_ERROR("Session monitor is required but not available");
        initSuccess = false;
    }

    if (m_trackSystemMetrics && m_systemMonitor) {
        if (!m_systemMonitor->initialize()) {
            LOG_ERROR("Failed to initialize system monitor");
            initSuccess = false;
        }
    }

    if (initSuccess) {
        LOG_INFO("MonitorManager initialized successfully");
    } else {
        LOG_ERROR("MonitorManager initialization failed");
    }

    return initSuccess;
}

bool MonitorManager::start()
{
    if (m_isRunning) {
        LOG_WARNING("MonitorManager is already running");
        return true;
    }

    LOG_INFO("Starting MonitorManager");

    bool startSuccess = true;

    // Start monitors according to tracking settings
    if (m_trackKeyboardMouse && m_keyboardMouseMonitor) {
        if (!m_keyboardMouseMonitor->start()) {
            LOG_ERROR("Failed to start keyboard/mouse monitor");
            startSuccess = false;
        }
    }

    if (m_trackApplications && m_appMonitor) {
        if (!m_appMonitor->start()) {
            LOG_ERROR("Failed to start app monitor");
            startSuccess = false;
        }
    }

    // Session monitor is always required
    if (m_sessionMonitor) {
        if (!m_sessionMonitor->start()) {
            LOG_ERROR("Failed to start session monitor");
            startSuccess = false;
        }
    } else {
        LOG_ERROR("Session monitor is required but not available");
        startSuccess = false;
    }

    if (m_trackSystemMetrics && m_systemMonitor) {
        if (!m_systemMonitor->start()) {
            LOG_ERROR("Failed to start system monitor");
            startSuccess = false;
        }
    }

    if (startSuccess) {
        m_isRunning = true;
        LOG_INFO("MonitorManager started successfully");
    } else {
        LOG_ERROR("MonitorManager start failed");
        // Cleanup: Stop any monitors that did start
        stop();
    }

    return startSuccess;
}

bool MonitorManager::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("MonitorManager is not running");
        return true;
    }

    LOG_INFO("Stopping MonitorManager");

    bool stopSuccess = true;

    // Stop all monitors that are active
    if (m_keyboardMouseMonitor) {
        if (!m_keyboardMouseMonitor->stop()) {
            LOG_ERROR("Failed to stop keyboard/mouse monitor");
            stopSuccess = false;
        }
    }

    if (m_appMonitor) {
        if (!m_appMonitor->stop()) {
            LOG_ERROR("Failed to stop app monitor");
            stopSuccess = false;
        }
    }

    if (m_sessionMonitor) {
        if (!m_sessionMonitor->stop()) {
            LOG_ERROR("Failed to stop session monitor");
            stopSuccess = false;
        }
    }

    if (m_systemMonitor) {
        if (!m_systemMonitor->stop()) {
            LOG_ERROR("Failed to stop system monitor");
            stopSuccess = false;
        }
    }

    m_isRunning = false;

    if (stopSuccess) {
        LOG_INFO("MonitorManager stopped successfully");
    } else {
        LOG_ERROR("MonitorManager stop had errors");
    }

    return stopSuccess;
}

bool MonitorManager::isRunning() const
{
    return m_isRunning;
}

void MonitorManager::connectMonitorSignals(ActivityMonitorBatcher* batcher)
{
    if (batcher) {
        // Connect keyboard/mouse monitor signals to batcher
        if (m_keyboardMouseMonitor) {
            connect(m_keyboardMouseMonitor, &KeyboardMouseMonitor::keyboardActivity,
                    batcher, &ActivityMonitorBatcher::addKeyboardEvent);
            connect(m_keyboardMouseMonitor, &KeyboardMouseMonitor::mouseActivity,
                    batcher, &ActivityMonitorBatcher::addMouseEvent);
        }

        // Connect app monitor signals to batcher
        if (m_appMonitor) {
            connect(m_appMonitor, &AppMonitor::appChanged,
                    batcher, &ActivityMonitorBatcher::addAppEvent);
        }
    }

    // Connect system monitor signals directly to our signals
    if (m_systemMonitor) {
        connect(m_systemMonitor, &SystemMonitor::systemMetricsUpdated,
                this, &MonitorManager::systemMetricsUpdated);
        connect(m_systemMonitor, &SystemMonitor::highCpuProcessDetected,
                this, &MonitorManager::highCpuProcessDetected);
    }

    // Connect session monitor signals directly to our signals
    if (m_sessionMonitor) {
        connect(m_sessionMonitor, &SessionMonitor::sessionStateChanged,
                this, &MonitorManager::sessionStateChanged);
        connect(m_sessionMonitor, &SessionMonitor::afkStateChanged,
                this, &MonitorManager::afkStateChanged);
    }
}

void MonitorManager::setIdleTimeThreshold(int milliseconds)
{
    if (m_keyboardMouseMonitor) {
        m_keyboardMouseMonitor->setIdleTimeThreshold(milliseconds);
    }
}

void MonitorManager::setHighCpuThreshold(float percentage)
{
    if (m_systemMonitor) {
        m_systemMonitor->setHighCpuThreshold(percentage);
    }
}

void MonitorManager::createPlatformMonitors()
{
    // Create platform-specific monitors based on OS at runtime
#ifdef Q_OS_WIN
    m_keyboardMouseMonitor = new KeyboardMouseMonitorWin(this);
    m_appMonitor = new AppMonitorWin(this);
    m_sessionMonitor = new SessionMonitorWin(this);
    m_systemMonitor = new SystemMonitorWin(this);
#elif defined(Q_OS_MACOS)
    m_keyboardMouseMonitor = new KeyboardMouseMonitorMac(this);
    m_appMonitor = new AppMonitorMac(this);
    m_sessionMonitor = new SessionMonitorMac(this);
    m_systemMonitor = new SystemMonitorMac(this);
#elif defined(Q_OS_LINUX)
    m_keyboardMouseMonitor = new KeyboardMouseMonitorLinux(this);
    m_appMonitor = new AppMonitorLinux(this);
    m_sessionMonitor = new SessionMonitorLinux(this);
    m_systemMonitor = new SystemMonitorLinux(this);
#endif
}
