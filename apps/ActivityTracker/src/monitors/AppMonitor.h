#ifndef APPMONITOR_H
#define APPMONITOR_H

#include <QObject>
#include <QString>

class AppMonitor : public QObject
{
    Q_OBJECT
public:
    explicit AppMonitor(QObject *parent = nullptr);
    virtual ~AppMonitor();

    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;

    // Get current active app details
    virtual QString currentAppName() const = 0;
    virtual QString currentWindowTitle() const = 0;
    virtual QString currentAppPath() const = 0;

    signals:
        void appChanged(const QString &appName, const QString &windowTitle, const QString &executablePath);
    void appFocused(const QString &appName, const QString &windowTitle, const QString &executablePath);
    void appUnfocused(const QString &appName, const QString &windowTitle, const QString &executablePath);
};

#endif // APPMONITOR_H