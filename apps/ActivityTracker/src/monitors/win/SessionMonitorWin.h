#ifndef SESSIONMONITORWIN_H
#define SESSIONMONITORWIN_H

#include "../SessionMonitor.h"
#include <Windows.h>
#include <WtsApi32.h>
#include <QMutex>

#pragma comment(lib, "wtsapi32.lib")

class SessionMonitorWin : public SessionMonitor
{
    Q_OBJECT
public:
    explicit SessionMonitorWin(QObject *parent = nullptr);
    ~SessionMonitorWin() override;

    bool initialize() override;
    bool start() override;
    bool stop() override;

    SessionState currentSessionState() const override;
    QString currentUser() const override;
    bool isRemoteSession() const override;

private:
    bool m_isRunning;
    SessionState m_currentState;
    QString m_currentUser;
    bool m_isRemoteSession;
    DWORD m_currentSessionId;
    QMutex m_mutex;
    
    // Message window for WTS notifications
    HWND m_messageWindow;
    
    // Register for session notifications
    bool registerForSessionNotifications();
    void unregisterForSessionNotifications();
    
    // Window procedure for handling messages
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Process session notifications
    void processSessionNotification(WPARAM wParam, LPARAM lParam);
    
    // Update session info
    void updateSessionInfo();
    
    // Helper methods
    QString getSessionUser(DWORD sessionId) const;
    bool isSessionRemote(DWORD sessionId) const;
    
    // Static instance for callback
    static SessionMonitorWin* s_instance;
};

#endif // SESSIONMONITORWIN_H