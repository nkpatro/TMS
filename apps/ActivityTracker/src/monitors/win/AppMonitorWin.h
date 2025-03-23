#ifndef APPMONITORWIN_H
#define APPMONITORWIN_H

#include "../AppMonitor.h"
#include <Windows.h>
#include <QFileInfo>
#include <QMutex>

class AppMonitorWin : public AppMonitor
{
    Q_OBJECT
public:
    explicit AppMonitorWin(QObject *parent = nullptr);
    ~AppMonitorWin() override;

    bool initialize() override;
    bool start() override;
    bool stop() override;

    QString currentAppName() const override;
    QString currentWindowTitle() const override;
    QString currentAppPath() const override;

private:
    bool m_isRunning;
    HWND m_currentWindow;
    QString m_currentAppName;
    QString m_currentWindowTitle;
    QString m_currentAppPath;
    HWINEVENTHOOK m_foregroundWinEventHook;
    QMutex m_mutex;
    
    // Helper methods for window info
    QString getWindowTitle(HWND hwnd) const;
    QString getAppName(HWND hwnd) const;
    QString getAppPath(HWND hwnd) const;
    void updateWindowInfo(HWND hwnd);
    
    // Static callback for WinEvent hook
    static void CALLBACK WinEventProc(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD dwEventThread,
        DWORD dwmsEventTime
    );
    
    // Static instance for callback
    static AppMonitorWin* s_instance;
};

#endif // APPMONITORWIN_H