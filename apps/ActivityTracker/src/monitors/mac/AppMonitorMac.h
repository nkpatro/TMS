// AppMonitorMac.h
#ifndef APPMONITORMAC_H
#define APPMONITORMAC_H

#include "../AppMonitor.h"
#include <QTimer>
#include <QString>

#ifdef __OBJC__
@class AppMonitorDelegate;
#else
typedef struct objc_object AppMonitorDelegate;
#endif

class AppMonitorMac : public AppMonitor
{
    Q_OBJECT
public:
    explicit AppMonitorMac(QObject *parent = nullptr);
    ~AppMonitorMac() override;

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
    QString m_currentAppName;
    QString m_currentWindowTitle;
    QString m_currentAppPath;

    // Objective-C delegate for NSWorkspace notifications
    AppMonitorDelegate* m_delegate;

    // Private methods
    QString getWindowTitle() const;
    QString getAppName() const;
    QString getAppPath() const;
    void updateWindowInfo();
};

#endif // APPMONITORMAC_H