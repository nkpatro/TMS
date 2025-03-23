#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QObject>
#include <QString>
#include <QList>
#include <QPair>

class SystemMonitor : public QObject
{
    Q_OBJECT
public:
    struct ProcessInfo {
        QString name;
        QString executablePath;
        qint64 pid;
        float cpuUsage;
        float memoryUsage;
    };

    explicit SystemMonitor(QObject *parent = nullptr);
    virtual ~SystemMonitor();

    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;

    // System information
    virtual float cpuUsage() const = 0;
    virtual float gpuUsage() const = 0;
    virtual float memoryUsage() const = 0;
    virtual QList<ProcessInfo> runningProcesses() const = 0;

    void setHighCpuThreshold(float percentage);
    float highCpuThreshold() const;

    signals:
        void systemMetricsUpdated(float cpuUsage, float gpuUsage, float ramUsage);
    void highCpuProcessDetected(const QString &processName, float cpuUsage);

protected:
    float m_highCpuThreshold;  // percentage
};

#endif // SYSTEMMONITOR_H