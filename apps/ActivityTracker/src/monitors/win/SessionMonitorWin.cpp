#include "SessionMonitorWin.h"
#include "logger/logger.h"

// Initialize static instance
SessionMonitorWin* SessionMonitorWin::s_instance = nullptr;

// Window class name for message window
const wchar_t* const WINDOW_CLASS_NAME = L"ActivityTrackerSessionMonitorClass";

SessionMonitorWin::SessionMonitorWin(QObject *parent)
    : SessionMonitor(parent)
    , m_isRunning(false)
    , m_currentState(SessionState::Unknown)
    , m_isRemoteSession(false)
    , m_currentSessionId(0)
    , m_messageWindow(NULL)
{
    s_instance = this;
}

SessionMonitorWin::~SessionMonitorWin()
{
    if (m_isRunning) {
        stop();
    }

    // Unregister window class
    if (m_messageWindow) {
        DestroyWindow(m_messageWindow);
        m_messageWindow = NULL;

        UnregisterClassW(WINDOW_CLASS_NAME, GetModuleHandle(NULL));
    }

    s_instance = nullptr;
}

bool SessionMonitorWin::initialize()
{
    LOG_INFO("Initializing SessionMonitorWin");

    // Create message window for receiving session notifications
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassExW(&wc)) {
        LOG_ERROR(QString("Failed to register window class, error code: %1").arg(GetLastError()));
        return false;
    }

    m_messageWindow = CreateWindowExW(
        0,                          // Extended style
        WINDOW_CLASS_NAME,          // Class name
        L"ActivityTrackerSession",  // Window name
        0,                          // Style
        0, 0, 0, 0,                 // Position and size (not visible)
        HWND_MESSAGE,               // Message-only window
        NULL,                       // No menu
        GetModuleHandle(NULL),      // Instance
        NULL                        // No creation parameters
    );

    if (!m_messageWindow) {
        LOG_ERROR(QString("Failed to create message window, error code: %1").arg(GetLastError()));
        UnregisterClassW(WINDOW_CLASS_NAME, GetModuleHandle(NULL));
        return false;
    }

    // Get initial session state
    updateSessionInfo();

    return true;
}

bool SessionMonitorWin::start()
{
    if (m_isRunning) {
        LOG_WARNING("SessionMonitorWin is already running");
        return true;
    }

    LOG_INFO("Starting SessionMonitorWin");

    // Register for session notifications
    if (!registerForSessionNotifications()) {
        LOG_ERROR("Failed to register for session notifications");
        return false;
    }

    m_isRunning = true;
    LOG_INFO("SessionMonitorWin started successfully");
    return true;
}

bool SessionMonitorWin::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("SessionMonitorWin is not running");
        return true;
    }

    LOG_INFO("Stopping SessionMonitorWin");

    // Unregister from session notifications
    unregisterForSessionNotifications();

    m_isRunning = false;
    LOG_INFO("SessionMonitorWin stopped successfully");
    return true;
}

SessionMonitor::SessionState SessionMonitorWin::currentSessionState() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_currentState;
}

QString SessionMonitorWin::currentUser() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_currentUser;
}

bool SessionMonitorWin::isRemoteSession() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_isRemoteSession;
}

bool SessionMonitorWin::registerForSessionNotifications()
{
    if (!m_messageWindow) {
        LOG_ERROR("Message window not created");
        return false;
    }

    // Register for Terminal Services session notifications
    if (!WTSRegisterSessionNotification(m_messageWindow, NOTIFY_FOR_ALL_SESSIONS)) {
        LOG_ERROR(QString("Failed to register for session notifications, error code: %1").arg(GetLastError()));
        return false;
    }

    return true;
}

void SessionMonitorWin::unregisterForSessionNotifications()
{
    if (m_messageWindow) {
        WTSUnRegisterSessionNotification(m_messageWindow);
    }
}

LRESULT CALLBACK SessionMonitorWin::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Handle WTS session notifications
    if (msg == WM_WTSSESSION_CHANGE && s_instance) {
        // FIX: Don't capture s_instance, use it directly in the lambda
        // since it's a static member variable
        QMetaObject::invokeMethod(s_instance, [wParam, lParam]() {
            // s_instance is available here without capturing it
            if (s_instance) {
                s_instance->processSessionNotification(wParam, lParam);
            }
        }, Qt::QueuedConnection);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void SessionMonitorWin::processSessionNotification(WPARAM wParam, LPARAM lParam)
{
    DWORD sessionId = static_cast<DWORD>(lParam);

    switch (wParam) {
        case WTS_CONSOLE_CONNECT:
            // Console session connected - user logged in locally
            {
                QMutexLocker locker(&m_mutex);

                QString user = getSessionUser(sessionId);
                m_currentSessionId = sessionId;
                m_currentUser = user;
                m_isRemoteSession = false;
                m_currentState = SessionState::Login;

                locker.unlock();
                // Use static_cast<int> to convert enum to int for signal
                emit sessionStateChanged(static_cast<int>(SessionState::Login), user);
            }
            break;

        case WTS_CONSOLE_DISCONNECT:
            // Console session disconnected - user logged out locally
            {
                QMutexLocker locker(&m_mutex);

                QString user = m_currentUser; // Save current user before clearing
                m_currentState = SessionState::Logout;

                locker.unlock();
                // Use static_cast<int> to convert enum to int for signal
                emit sessionStateChanged(static_cast<int>(SessionState::Logout), user);
            }
            break;

        case WTS_REMOTE_CONNECT:
            // Remote session connected
            {
                QMutexLocker locker(&m_mutex);

                QString user = getSessionUser(sessionId);
                m_currentSessionId = sessionId;
                m_currentUser = user;
                m_isRemoteSession = true;
                m_currentState = SessionState::RemoteConnect;

                locker.unlock();
                // Use static_cast<int> to convert enum to int for signal
                emit sessionStateChanged(static_cast<int>(SessionState::RemoteConnect), user);
            }
            break;

        case WTS_REMOTE_DISCONNECT:
            // Remote session disconnected
            {
                QMutexLocker locker(&m_mutex);

                QString user = m_currentUser; // Save current user before clearing
                m_currentState = SessionState::RemoteDisconnect;

                locker.unlock();
                emit sessionStateChanged(static_cast<int>(SessionState::RemoteDisconnect), user);
            }
            break;

        case WTS_SESSION_LOCK:
            // Session locked
            {
                QMutexLocker locker(&m_mutex);

                QString user = m_currentUser;
                m_currentState = SessionState::Lock;

                locker.unlock();
                emit sessionStateChanged(static_cast<int>(SessionState::Lock), user);
                emit afkStateChanged(true);
            }
            break;

        case WTS_SESSION_UNLOCK:
            // Session unlocked
            {
                QMutexLocker locker(&m_mutex);

                QString user = m_currentUser;
                m_currentState = SessionState::Unlock;

                locker.unlock();
                emit sessionStateChanged(static_cast<int>(SessionState::Unlock), user);
                emit afkStateChanged(false);
            }
            break;

        case WTS_SESSION_LOGON:
            // User logged on (may be duplicate of CONSOLE_CONNECT)
            {
                QMutexLocker locker(&m_mutex);

                QString user = getSessionUser(sessionId);
                if (user != m_currentUser) {
                    // User changed
                    m_currentUser = user;
                    m_currentSessionId = sessionId;
                    m_currentState = SessionState::SwitchUser;

                    locker.unlock();
                    emit sessionStateChanged(static_cast<int>(SessionState::SwitchUser), user);
                }
            }
            break;

        case WTS_SESSION_LOGOFF:
            // User logged off (may be duplicate of CONSOLE_DISCONNECT)
            {
                QMutexLocker locker(&m_mutex);

                QString user = m_currentUser; // Save current user before clearing
                m_currentState = SessionState::Logout;

                locker.unlock();
                emit sessionStateChanged(static_cast<int>(SessionState::Logout), user);
            }
            break;
    }
}

void SessionMonitorWin::updateSessionInfo()
{
    QMutexLocker locker(&m_mutex);

    // Get current session ID
    DWORD sessionId = WTSGetActiveConsoleSessionId();
    if (sessionId == 0xFFFFFFFF) {
        // No active session
        m_currentState = SessionState::Unknown;
        m_currentUser = "";
        m_isRemoteSession = false;
        return;
    }

    m_currentSessionId = sessionId;
    m_currentUser = getSessionUser(sessionId);
    m_isRemoteSession = isSessionRemote(sessionId);

    // Current state is determined by session type
    if (m_currentUser.isEmpty()) {
        m_currentState = SessionState::Logout;
    } else if (m_isRemoteSession) {
        m_currentState = SessionState::RemoteConnect;
    } else {
        m_currentState = SessionState::Login;
    }
}

QString SessionMonitorWin::getSessionUser(DWORD sessionId) const
{
    LPWSTR userName = nullptr;
    DWORD bytesReturned = 0;

    if (WTSQuerySessionInformationW(
            WTS_CURRENT_SERVER_HANDLE,
            sessionId,
            WTSUserName,
            &userName,
            &bytesReturned) && userName) {

        QString user = QString::fromWCharArray(userName);
        WTSFreeMemory(userName);
        return user;
    }

    return "";
}

bool SessionMonitorWin::isSessionRemote(DWORD sessionId) const
{
    BOOL* isRemote = nullptr;
    DWORD bytesReturned = 0;

    if (WTSQuerySessionInformationW(
            WTS_CURRENT_SERVER_HANDLE,
            sessionId,
            WTSClientProtocolType,
            (LPWSTR*)&isRemote,
            &bytesReturned) && isRemote) {

        bool remote = (*isRemote != 0);
        WTSFreeMemory(isRemote);
        return remote;
    }

    return false;
}
