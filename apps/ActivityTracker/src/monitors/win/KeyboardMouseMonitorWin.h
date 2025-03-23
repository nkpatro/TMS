// KeyboardMouseMonitorWin.h
#ifndef KEYBOARDMOUSEMONITORWIN_H
#define KEYBOARDMOUSEMONITORWIN_H

#include "../KeyboardMouseMonitor.h"
#include <QTimer>
#include <QMutex>
#include <Windows.h>

class KeyboardMouseMonitorWin : public KeyboardMouseMonitor
{
    Q_OBJECT
public:
    explicit KeyboardMouseMonitorWin(QObject *parent = nullptr);
    ~KeyboardMouseMonitorWin() override;

    bool initialize() override;
    bool start() override;
    bool stop() override;
    int getIdleTime() const override;

    private slots:
        void checkIdleTime();
    void processEvents();

private:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

    static KeyboardMouseMonitorWin* s_instance;
    HHOOK m_keyboardHook;
    HHOOK m_mouseHook;
    QTimer m_idleTimer;
    QTimer m_processEventsTimer;
    bool m_isRunning;
    bool m_isIdle;
    QMutex m_mutex;
};

#endif // KEYBOARDMOUSEMONITORWIN_H