#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <QObject>
#include <QString>
#include <QHostAddress>
#include <QSysInfo>
#include <QList>
#include <QPair>
#include <QJsonObject>

class SystemInfo : public QObject
{
    Q_OBJECT
public:
    explicit SystemInfo(QObject *parent = nullptr);

    // System identification
    static QString getMachineHostName();
    static QString getMachineUniqueId();
    static QString getMacAddress();
    static QHostAddress getLocalIPAddress();

    // Operating system information
    static QString getOperatingSystem();
    static QString getOSVersion();
    static QString getKernelVersion();

    // Hardware information
    static QString getCPUInfo();
    static QString getGPUInfo();
    static int getTotalRAMGB();

    // System metrics
    static double getCurrentCPUUsage();
    static double getCurrentMemoryUsage();
    static double getCurrentGPUUsage();

    // Machine fingerprint
    static QString generateMachineFingerprint();

    // Return all system information as a JSON object
    static QJsonObject getAllSystemInfo();

private:
    // Helper methods
    static QList<QPair<QString, QString>> getCPUInfoPairs();
    static QList<QPair<QString, QString>> getGPUInfoPairs();
    static bool readCPUStatistics(quint64 &totalUser, quint64 &totalUserLow, quint64 &totalSys, quint64 &totalIdle);
};

#endif // SYSTEMINFO_H
