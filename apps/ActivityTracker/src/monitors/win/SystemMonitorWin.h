// SystemMonitorWin.h
#ifndef SYSTEMMONITORWIN_H
#define SYSTEMMONITORWIN_H

#include "../SystemMonitor.h"
#include <QTimer>
#include <Windows.h>
#include <pdh.h>
#include <psapi.h>

class SystemMonitorWin : public SystemMonitor
{
    Q_OBJECT
public:
    explicit SystemMonitorWin(QObject *parent = nullptr);
    ~SystemMonitorWin() override;

    bool initialize() override;
    bool start() override;
    bool stop() override;

    float cpuUsage() const override;
    float gpuUsage() const override;
    float memoryUsage() const override;
    QList<ProcessInfo> runningProcesses() const override;

    private slots:
        void updateMetrics();

private:
    QTimer m_updateTimer;
    bool m_isRunning;

    float m_cpuUsage;
    float m_gpuUsage;
    float m_memoryUsage;
    QList<ProcessInfo> m_processes;

    // PDH (Performance Data Helper) handles
    PDH_HQUERY m_cpuQuery;
    PDH_HCOUNTER m_cpuCounter;
    PDH_HQUERY m_gpuQuery;
    PDH_HCOUNTER m_gpuCounter;

    bool initializePdhCounters();
    void cleanupPdhCounters();
    void updateCpuUsage();
    void updateGpuUsage();
    void updateMemoryUsage();
    void updateProcessList();
    double calculateProcessCpuUsage(HANDLE processHandle, ULARGE_INTEGER &lastProcessTime, ULARGE_INTEGER &lastSystemTime);
};

#endif // SYSTEMMONITORWIN_H