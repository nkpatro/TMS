#include "AppMonitorWin.h"
#include "logger/logger.h"
#include <psapi.h>

// Initialize static instance pointer
AppMonitorWin* AppMonitorWin::s_instance = nullptr;

AppMonitorWin::AppMonitorWin(QObject *parent)
    : AppMonitor(parent)
    , m_isRunning(false)
    , m_currentWindow(NULL)
    , m_foregroundWinEventHook(NULL)
{
    s_instance = this;
}

AppMonitorWin::~AppMonitorWin()
{
    if (m_isRunning) {
        stop();
    }
    s_instance = nullptr;
}

bool AppMonitorWin::initialize()
{
    LOG_INFO("Initializing AppMonitorWin");
    return true;
}

bool AppMonitorWin::start()
{
    if (m_isRunning) {
        LOG_WARNING("AppMonitorWin is already running");
        return true;
    }

    LOG_INFO("Starting AppMonitorWin");

    // Set up Windows event hook for foreground window changes
    m_foregroundWinEventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,     // Event to hook
        EVENT_SYSTEM_FOREGROUND,     // Only this event
        NULL,                        // No DLL - use current process
        WinEventProc,                // Callback function
        0,                           // Hook all processes
        0,                           // Hook all threads
        WINEVENT_OUTOFCONTEXT        // Out-of-context (async) mode
    );

    if (!m_foregroundWinEventHook) {
        LOG_ERROR(QString("Failed to set foreground window event hook, error code: %1")
                     .arg(GetLastError()));
        return false;
    }

    // Initialize with current window
    updateWindowInfo(GetForegroundWindow());

    m_isRunning = true;
    LOG_INFO("AppMonitorWin started successfully");
    return true;
}

bool AppMonitorWin::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("AppMonitorWin is not running");
        return true;
    }

    LOG_INFO("Stopping AppMonitorWin");

    // Unhook the event
    if (m_foregroundWinEventHook) {
        UnhookWinEvent(m_foregroundWinEventHook);
        m_foregroundWinEventHook = NULL;
    }

    m_isRunning = false;
    LOG_INFO("AppMonitorWin stopped successfully");
    return true;
}

QString AppMonitorWin::currentAppName() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_currentAppName;
}

QString AppMonitorWin::currentWindowTitle() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_currentWindowTitle;
}

QString AppMonitorWin::currentAppPath() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_currentAppPath;
}

QString AppMonitorWin::getWindowTitle(HWND hwnd) const
{
    if (!hwnd) return "";

    int length = GetWindowTextLengthW(hwnd);
    if (length == 0) return "";

    std::wstring title(length + 1, L'\0');
    GetWindowTextW(hwnd, &title[0], length + 1);
    title.resize(length);

    return QString::fromStdWString(title);
}

QString AppMonitorWin::getAppName(HWND hwnd) const
{
    QString path = getAppPath(hwnd);
    if (path.isEmpty()) return "";

    QFileInfo fileInfo(path);
    return fileInfo.baseName();
}

QString AppMonitorWin::getAppPath(HWND hwnd) const
{
    if (!hwnd) return "";

    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);

    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!processHandle) return "";

    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;
    BOOL result = QueryFullProcessImageNameW(processHandle, 0, path, &size);
    CloseHandle(processHandle);

    if (!result) return "";

    return QString::fromWCharArray(path);
}

void AppMonitorWin::updateWindowInfo(HWND hwnd)
{
    if (!hwnd) return;

    QMutexLocker locker(&m_mutex);

    QString windowTitle = getWindowTitle(hwnd);
    QString appPath = getAppPath(hwnd);
    QString appName = getAppName(hwnd);

    // Only update and emit signals if there's a real change
    if (m_currentWindow != hwnd ||
        m_currentWindowTitle != windowTitle ||
        m_currentAppPath != appPath ||
        m_currentAppName != appName) {

        // Emit signal about previous window losing focus
        if (m_currentWindow != NULL && !m_currentAppName.isEmpty()) {
            locker.unlock();
            emit appUnfocused(m_currentAppName, m_currentWindowTitle, m_currentAppPath);
            locker.relock();
        }

        // Update current window info
        m_currentWindow = hwnd;
        m_currentWindowTitle = windowTitle;
        m_currentAppPath = appPath;
        m_currentAppName = appName;

        // Emit signals about new window
        if (!m_currentAppName.isEmpty()) {
            locker.unlock();
            emit appFocused(m_currentAppName, m_currentWindowTitle, m_currentAppPath);
            emit appChanged(m_currentAppName, m_currentWindowTitle, m_currentAppPath);
        }
    }
}

void CALLBACK AppMonitorWin::WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime)
{
    // Ensure this is a foreground window change and we have a valid instance
    if (event == EVENT_SYSTEM_FOREGROUND && s_instance && idObject == OBJID_WINDOW) {
        // FIX: Don't capture s_instance in the lambda, use it directly
        // since it's a static member variable
        QMetaObject::invokeMethod(s_instance, [hwnd]() {
            // s_instance is available here without capturing it
            if (s_instance) {
                s_instance->updateWindowInfo(hwnd);
            }
        }, Qt::QueuedConnection);
    }
}