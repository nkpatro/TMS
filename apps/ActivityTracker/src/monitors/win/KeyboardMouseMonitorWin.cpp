#include "KeyboardMouseMonitorWin.h"
#include "logger/logger.h"

// Initialize static instance pointer
KeyboardMouseMonitorWin* KeyboardMouseMonitorWin::s_instance = nullptr;

KeyboardMouseMonitorWin::KeyboardMouseMonitorWin(QObject *parent)
    : KeyboardMouseMonitor(parent)
    , m_keyboardHook(NULL)
    , m_mouseHook(NULL)
    , m_isRunning(false)
    , m_isIdle(false)
{
    s_instance = this;

    // Set up idle timer for checking if user has been idle
    connect(&m_idleTimer, &QTimer::timeout, this, &KeyboardMouseMonitorWin::checkIdleTime);
    m_idleTimer.setInterval(5000); // Check idle time every 5 seconds

    // Timer to process Windows messages regularly
    connect(&m_processEventsTimer, &QTimer::timeout, this, &KeyboardMouseMonitorWin::processEvents);
    m_processEventsTimer.setInterval(100); // 10 times per second
}

KeyboardMouseMonitorWin::~KeyboardMouseMonitorWin()
{
    if (m_isRunning) {
        stop();
    }

    s_instance = nullptr;
}

bool KeyboardMouseMonitorWin::initialize()
{
    LOG_INFO("Initializing KeyboardMouseMonitorWin");
    return true;
}

bool KeyboardMouseMonitorWin::start()
{
    if (m_isRunning) {
        LOG_WARNING("KeyboardMouseMonitorWin is already running");
        return true;
    }

    LOG_INFO("Starting KeyboardMouseMonitorWin");

    // Set up keyboard hook
    m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                                     GetModuleHandle(NULL), 0);
    if (!m_keyboardHook) {
        LOG_ERROR(QString("Failed to set keyboard hook, error code: %1")
                     .arg(GetLastError()));
        return false;
    }

    // Set up mouse hook
    m_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc,
                                  GetModuleHandle(NULL), 0);
    if (!m_mouseHook) {
        LOG_ERROR(QString("Failed to set mouse hook, error code: %1")
                     .arg(GetLastError()));
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = NULL;
        return false;
    }

    // Start timers
    m_idleTimer.start();
    m_processEventsTimer.start();
    m_isRunning = true;

    LOG_INFO("KeyboardMouseMonitorWin started successfully");
    return true;
}

bool KeyboardMouseMonitorWin::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("KeyboardMouseMonitorWin is not running");
        return true;
    }

    LOG_INFO("Stopping KeyboardMouseMonitorWin");

    // Stop timers
    m_idleTimer.stop();
    m_processEventsTimer.stop();

    // Remove hooks
    if (m_keyboardHook) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = NULL;
    }

    if (m_mouseHook) {
        UnhookWindowsHookEx(m_mouseHook);
        m_mouseHook = NULL;
    }

    m_isRunning = false;
    LOG_INFO("KeyboardMouseMonitorWin stopped successfully");
    return true;
}

int KeyboardMouseMonitorWin::getIdleTime() const
{
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii)) {
        DWORD tickCount = GetTickCount();
        DWORD lastInputTick = lii.dwTime;
        return (tickCount - lastInputTick);
    }
    return 0;
}

void KeyboardMouseMonitorWin::checkIdleTime()
{
    int idleTime = getIdleTime();
    bool shouldBeIdle = (idleTime >= m_idleTimeThreshold);

    if (shouldBeIdle != m_isIdle) {
        m_isIdle = shouldBeIdle;
        if (m_isIdle) {
            emit idleTimeExceeded();
        } else {
            emit userReturnedFromIdle();
        }
    }
}

void KeyboardMouseMonitorWin::processEvents()
{
    // Process Windows messages to keep hooks responsive
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK KeyboardMouseMonitorWin::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        if (s_instance) {
            QMetaObject::invokeMethod(s_instance, "keyboardActivity", Qt::QueuedConnection);
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardMouseMonitorWin::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* mouseStruct = (MSLLHOOKSTRUCT*)lParam;
        bool isClick = (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN ||
                        wParam == WM_MBUTTONDOWN);

        if (s_instance) {
            QMetaObject::invokeMethod(s_instance, "mouseActivity", Qt::QueuedConnection,
                                     Q_ARG(int, mouseStruct->pt.x),
                                     Q_ARG(int, mouseStruct->pt.y),
                                     Q_ARG(bool, isClick));
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}