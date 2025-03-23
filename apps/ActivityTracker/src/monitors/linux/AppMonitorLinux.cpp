#include "AppMonitorLinux.h"
#include "../../utils/Logger.h"
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <unistd.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

AppMonitorLinux::AppMonitorLinux(QObject *parent)
    : AppMonitor(parent)
    , m_isRunning(false)
    , m_display(nullptr)
    , m_currentWindow(0)
{
    // Set up polling timer for checking active window
    connect(&m_pollTimer, &QTimer::timeout, this, &AppMonitorLinux::checkActiveWindow);
    m_pollTimer.setInterval(1000); // Check active window every second
}

AppMonitorLinux::~AppMonitorLinux()
{
    if (m_isRunning) {
        stop();
    }

    cleanupX11();
}

bool AppMonitorLinux::initialize()
{
    LOG_INFO("Initializing AppMonitorLinux");

    // Initialize X11 connection
    if (!initializeX11()) {
        LOG_ERROR("Failed to initialize X11 connection");
        return false;
    }

    return true;
}

bool AppMonitorLinux::start()
{
    if (m_isRunning) {
        LOG_WARNING("AppMonitorLinux is already running");
        return true;
    }

    LOG_INFO("Starting AppMonitorLinux");

    // Initialize with current window
    checkActiveWindow();

    // Start polling timer
    m_pollTimer.start();
    m_isRunning = true;

    LOG_INFO("AppMonitorLinux started successfully");
    return true;
}

bool AppMonitorLinux::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("AppMonitorLinux is not running");
        return true;
    }

    LOG_INFO("Stopping AppMonitorLinux");

    // Stop polling timer
    m_pollTimer.stop();
    m_isRunning = false;

    LOG_INFO("AppMonitorLinux stopped successfully");
    return true;
}

QString AppMonitorLinux::currentAppName() const
{
    return m_currentAppName;
}

QString AppMonitorLinux::currentWindowTitle() const
{
    return m_currentWindowTitle;
}

QString AppMonitorLinux::currentAppPath() const
{
    return m_currentAppPath;
}

void AppMonitorLinux::checkActiveWindow()
{
    if (!m_display) {
        LOG_ERROR("X11 display not initialized");
        return;
    }

    Window activeWindow = getActiveWindow();

    if (activeWindow != m_currentWindow) {
        // Window has changed, emit unfocus for old window
        if (m_currentWindow != 0 && !m_currentAppName.isEmpty()) {
            emit appUnfocused(m_currentAppName, m_currentWindowTitle, m_currentAppPath);
        }

        // Get info for new window
        updateWindowInfo(activeWindow);
        m_currentWindow = activeWindow;

        // Emit focus and change events
        if (!m_currentAppName.isEmpty()) {
            emit appFocused(m_currentAppName, m_currentWindowTitle, m_currentAppPath);
            emit appChanged(m_currentAppName, m_currentWindowTitle, m_currentAppPath);
        }
    }
}

Window AppMonitorLinux::getActiveWindow() const
{
    if (!m_display) return 0;

    Atom actualType;
    int actualFormat;
    unsigned long nitems, bytesAfter;
    unsigned char *data = nullptr;
    Window activeWindow = 0;

    // Get _NET_ACTIVE_WINDOW property of the root window
    int status = XGetWindowProperty(
        m_display,
        DefaultRootWindow(m_display),
        m_atomNetActiveWindow,
        0L,
        1L,
        False,
        XA_WINDOW,
        &actualType,
        &actualFormat,
        &nitems,
        &bytesAfter,
        &data
    );

    if (status == Success && data) {
        activeWindow = *reinterpret_cast<Window*>(data);
        XFree(data);
    }

    return activeWindow;
}

QString AppMonitorLinux::getWindowTitle(Window window) const
{
    if (!m_display || window == 0) return "";

    Atom actualType;
    int actualFormat;
    unsigned long nitems, bytesAfter;
    unsigned char *data = nullptr;
    QString title;

    // First try _NET_WM_NAME (UTF-8)
    int status = XGetWindowProperty(
        m_display,
        window,
        m_atomNetWmName,
        0L,
        (~0L),
        False,
        m_atomUtf8String,
        &actualType,
        &actualFormat,
        &nitems,
        &bytesAfter,
        &data
    );

    if (status == Success && data) {
        title = QString::fromUtf8(reinterpret_cast<char*>(data));
        XFree(data);
        return title;
    }

    // Fallback to WM_NAME
    status = XGetWindowProperty(
        m_display,
        window,
        m_atomWmName,
        0L,
        (~0L),
        False,
        XA_STRING,
        &actualType,
        &actualFormat,
        &nitems,
        &bytesAfter,
        &data
    );

    if (status == Success && data) {
        title = QString::fromLatin1(reinterpret_cast<char*>(data));
        XFree(data);
    }

    return title;
}

pid_t AppMonitorLinux::getWindowPid(Window window) const
{
    if (!m_display || window == 0) return 0;

    Atom atomPid = XInternAtom(m_display, "_NET_WM_PID", True);
    if (atomPid == None) return 0;

    Atom actualType;
    int actualFormat;
    unsigned long nitems, bytesAfter;
    unsigned char *data = nullptr;
    pid_t pid = 0;

    int status = XGetWindowProperty(
        m_display,
        window,
        atomPid,
        0L,
        1L,
        False,
        XA_CARDINAL,
        &actualType,
        &actualFormat,
        &nitems,
        &bytesAfter,
        &data
    );

    if (status == Success && data) {
        pid = *reinterpret_cast<pid_t*>(data);
        XFree(data);
    }

    return pid;
}

QString AppMonitorLinux::getAppName(Window window) const
{
    pid_t pid = getWindowPid(window);
    if (pid <= 0) return "";

    // Read process name from /proc/{pid}/comm
    QFile commFile(QString("/proc/%1/comm").arg(pid));
    if (commFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&commFile);
        QString name = stream.readLine().trimmed();
        commFile.close();
        return name;
    }

    return "";
}

QString AppMonitorLinux::getAppPath(Window window) const
{
    pid_t pid = getWindowPid(window);
    if (pid <= 0) return "";

    // Read executable path from /proc/{pid}/exe
    QString exePath = QString("/proc/%1/exe").arg(pid);

    // Resolve symlink
    QFileInfo fileInfo(exePath);
    return fileInfo.canonicalFilePath();
}

void AppMonitorLinux::updateWindowInfo(Window window)
{
    if (window) {
        m_currentWindowTitle = getWindowTitle(window);
        m_currentAppName = getAppName(window);
        m_currentAppPath = getAppPath(window);
    } else {
        m_currentWindowTitle = "";
        m_currentAppName = "";
        m_currentAppPath = "";
    }
}

bool AppMonitorLinux::initializeX11()
{
    // Open X11 display
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        LOG_ERROR("Failed to open X11 display");
        return false;
    }

    // Initialize atoms
    m_atomNetActiveWindow = XInternAtom(m_display, "_NET_ACTIVE_WINDOW", False);
    m_atomNetWmName = XInternAtom(m_display, "_NET_WM_NAME", False);
    m_atomWmName = XInternAtom(m_display, "WM_NAME", False);
    m_atomUtf8String = XInternAtom(m_display, "UTF8_STRING", False);

    LOG_DEBUG("X11 initialized successfully");
    return true;
}

void AppMonitorLinux::cleanupX11()
{
    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
}