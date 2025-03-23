// ServiceManager.h
#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QString>
#include "ActivityTrackerService.h"

class ServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManager(QObject *parent = nullptr);
    ~ServiceManager();

    bool installService();
    bool uninstallService();
    bool startService();
    bool stopService();
    bool runService(ActivityTrackerService &service);

private:
    QString serviceName() const;
    QString serviceDisplayName() const;
    QString serviceDescription() const;
    QString serviceExecutable() const;
};

#endif // SERVICEMANAGER_H