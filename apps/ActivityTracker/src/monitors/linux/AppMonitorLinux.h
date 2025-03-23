// AppMonitorLinux.h
#ifndef APPMONITORLINUX_H
#define APPMONITORLINUX_H

#include "../AppMonitor.h"
#include <QTimer>
#include <QString>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

class AppMonitorLinux : public AppMonitor
{
    Q_OBJECT
public:
    explicit AppMonitorLinux(QObject *parent = nullptr);
    ~AppMonitorLinux() override;

    bool initialize() override;
    bool start() override;
    bool stop() override;

    QString currentAppName() const override;
    QString currentWindowTitle() const override;
    QString currentAppPath() const override;

    private slots:
        void checkActiveWindow();

private:
    QTimer m_pollTimer;
    bool m_isRunning;
    Display* m_display;
    Window m_currentWindow;
    QString m_currentAppName;
    QString m_currentWindowTitle;
    QString m_currentAppPath;

    // X11 atoms
    Atom m_atomNetActiveWindow;
    Atom m_atomNetWmName;
    Atom m_atomWmName;
    Atom m_atomUtf8String;

    QString getWindowTitle(Window window) const;
    QString getAppName(Window window) const;
    QString getAppPath(Window window) const;
    Window getActiveWindow() const;
    pid_t getWindowPid(Window window) const;
    void updateWindowInfo(Window window);
    void cleanupX11();
    bool initializeX11();
};

#endif // APPMONITORLINUX_H