// SystemMonitorWin.cpp
#include "SystemMonitorWin.h"
#include "logger/logger.h"
#include <QFileInfo>

// Link with pdh.lib
#pragma comment(lib, "pdh.lib")

SystemMonitorWin::SystemMonitorWin(QObject *parent)
    : SystemMonitor(parent)
    , m_isRunning(false)
    , m_cpuUsage(0.0f)
    , m_gpuUsage(0.0f)
    , m_memoryUsage(0.0f)
    , m_cpuQuery(NULL)
    , m_cpuCounter(NULL)
    , m_gpuQuery(NULL)
    , m_gpuCounter(NULL)
{
    // Set up timer for polling system metrics
    connect(&m_updateTimer, &QTimer::timeout, this, &SystemMonitorWin::updateMetrics);
    m_updateTimer.setInterval(5000); // Update every 5 seconds
}

SystemMonitorWin::~SystemMonitorWin()
{
    if (m_isRunning) {
        stop();
    }
    cleanupPdhCounters();
}

bool SystemMonitorWin::initialize()
{
    LOG_INFO("Initializing SystemMonitorWin");

    if (!initializePdhCounters()) {
        LOG_WARNING("Failed to initialize PDH counters, some metrics may be unavailable");
    }

    // Initial update of metrics
    updateMetrics();

    return true;
}

bool SystemMonitorWin::start()
{
    if (m_isRunning) {
        LOG_WARNING("SystemMonitorWin is already running");
        return true;
    }

    LOG_INFO("Starting SystemMonitorWin");

    // Start the update timer
    m_updateTimer.start();
    m_isRunning = true;

    LOG_INFO("SystemMonitorWin started successfully");
    return true;
}

bool SystemMonitorWin::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("SystemMonitorWin is not running");
        return true;
    }

    LOG_INFO("Stopping SystemMonitorWin");

    // Stop the update timer
    m_updateTimer.stop();
    m_isRunning = false;

    LOG_INFO("SystemMonitorWin stopped successfully");
    return true;
}

float SystemMonitorWin::cpuUsage() const
{
    return m_cpuUsage;
}

float SystemMonitorWin::gpuUsage() const
{
    return m_gpuUsage;
}

float SystemMonitorWin::memoryUsage() const
{
    return m_memoryUsage;
}

QList<SystemMonitor::ProcessInfo> SystemMonitorWin::runningProcesses() const
{
    return m_processes;
}

void SystemMonitorWin::updateMetrics()
{
    updateCpuUsage();
    updateGpuUsage();
    updateMemoryUsage();
    updateProcessList();

    // Emit signal with updated metrics
    emit systemMetricsUpdated(m_cpuUsage, m_gpuUsage, m_memoryUsage);

    // Check for high CPU processes
    for (const auto& process : m_processes) {
        if (process.cpuUsage > m_highCpuThreshold) {
            emit highCpuProcessDetected(process.name, process.cpuUsage);
        }
    }
}

bool SystemMonitorWin::initializePdhCounters()
{
    PDH_STATUS status;

    // Initialize CPU counter
    status = PdhOpenQuery(NULL, 0, &m_cpuQuery);
    if (status != ERROR_SUCCESS) {
        LOG_ERROR(QString("Failed to open CPU query: %1").arg(status));
        return false;
    }

    // Add CPU counter
    status = PdhAddEnglishCounter(m_cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpuCounter);
    if (status != ERROR_SUCCESS) {
        LOG_ERROR(QString("Failed to add CPU counter: %1").arg(status));
        PdhCloseQuery(m_cpuQuery);
        m_cpuQuery = NULL;
        return false;
    }

    // Initialize GPU counter (may not be available on all systems)
    status = PdhOpenQuery(NULL, 0, &m_gpuQuery);
    if (status == ERROR_SUCCESS) {
        status = PdhAddEnglishCounter(m_gpuQuery, L"\\GPU Engine(*)\\Utilization Percentage", 0, &m_gpuCounter);
        if (status != ERROR_SUCCESS) {
            LOG_WARNING("GPU counter not available");
            PdhCloseQuery(m_gpuQuery);
            m_gpuQuery = NULL;
            m_gpuCounter = NULL;
        }
    } else {
        LOG_WARNING("GPU query not available");
        m_gpuQuery = NULL;
        m_gpuCounter = NULL;
    }

    // Collect initial data
    PdhCollectQueryData(m_cpuQuery);
    if (m_gpuQuery) {
        PdhCollectQueryData(m_gpuQuery);
    }

    return true;
}

void SystemMonitorWin::cleanupPdhCounters()
{
    if (m_cpuQuery) {
        PdhCloseQuery(m_cpuQuery);
        m_cpuQuery = NULL;
        m_cpuCounter = NULL;
    }

    if (m_gpuQuery) {
        PdhCloseQuery(m_gpuQuery);
        m_gpuQuery = NULL;
        m_gpuCounter = NULL;
    }
}

void SystemMonitorWin::updateCpuUsage()
{
    if (!m_cpuQuery || !m_cpuCounter) {
        m_cpuUsage = 0.0f;
        return;
    }

    PDH_FMT_COUNTERVALUE counterValue;
    PDH_STATUS status;

    // Collect new data
    status = PdhCollectQueryData(m_cpuQuery);
    if (status != ERROR_SUCCESS) {
        LOG_ERROR(QString("Failed to collect CPU data: %1").arg(status));
        m_cpuUsage = 0.0f;
        return;
    }

    // Get formatted counter value
    status = PdhGetFormattedCounterValue(m_cpuCounter, PDH_FMT_DOUBLE, NULL, &counterValue);
    if (status != ERROR_SUCCESS) {
        LOG_ERROR(QString("Failed to get CPU counter value: %1").arg(status));
        m_cpuUsage = 0.0f;
        return;
    }

    m_cpuUsage = static_cast<float>(counterValue.doubleValue);
}

void SystemMonitorWin::updateGpuUsage()
{
    if (!m_gpuQuery || !m_gpuCounter) {
        m_gpuUsage = 0.0f;
        return;
    }

    PDH_FMT_COUNTERVALUE counterValue;
    PDH_STATUS status;

    // Collect new data
    status = PdhCollectQueryData(m_gpuQuery);
    if (status != ERROR_SUCCESS) {
        LOG_ERROR(QString("Failed to collect GPU data: %1").arg(status));
        m_gpuUsage = 0.0f;
        return;
    }

    // Get formatted counter value
    status = PdhGetFormattedCounterValue(m_gpuCounter, PDH_FMT_DOUBLE, NULL, &counterValue);
    if (status != ERROR_SUCCESS) {
        LOG_ERROR(QString("Failed to get GPU counter value: %1").arg(status));
        m_gpuUsage = 0.0f;
        return;
    }

    m_gpuUsage = static_cast<float>(counterValue.doubleValue);
}

void SystemMonitorWin::updateMemoryUsage()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memInfo)) {
        // Calculate RAM usage as percentage
        m_memoryUsage = static_cast<float>(memInfo.dwMemoryLoad);
    } else {
        LOG_ERROR("Failed to get memory information");
        m_memoryUsage = 0.0f;
    }
}

void SystemMonitorWin::updateProcessList()
{
    // Clear previous process list
    m_processes.clear();

    // Get process IDs
    DWORD processes[1024];
    DWORD needed;
    if (!EnumProcesses(processes, sizeof(processes), &needed)) {
        LOG_ERROR("Failed to enumerate processes");
        return;
    }

    // Calculate number of processes
    DWORD numProcesses = needed / sizeof(DWORD);

    // System time values for CPU calculation
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);
    ULARGE_INTEGER sysTime;
    sysTime.LowPart = kernelTime.dwLowDateTime;  // FIXED: Use dwLowDateTime
    sysTime.HighPart = kernelTime.dwHighDateTime; // FIXED: Use dwHighDateTime

    ULARGE_INTEGER userTimeValue;
    userTimeValue.LowPart = userTime.dwLowDateTime;
    userTimeValue.HighPart = userTime.dwHighDateTime;
    sysTime.QuadPart += userTimeValue.QuadPart;

    // Process each process
    for (DWORD i = 0; i < numProcesses; i++) {
        if (processes[i] == 0) continue;

        // Open process
        HANDLE processHandle = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            FALSE, processes[i]);

        if (processHandle) {
            // Get process info
            ProcessInfo info;
            info.pid = processes[i];

            // Get process name
            WCHAR processName[MAX_PATH] = L"<unknown>";
            if (GetModuleBaseNameW(processHandle, NULL, processName, MAX_PATH)) {
                info.name = QString::fromWCharArray(processName);
            }

            // Get process path
            WCHAR processPath[MAX_PATH] = L"";
            if (GetModuleFileNameExW(processHandle, NULL, processPath, MAX_PATH)) {
                info.executablePath = QString::fromWCharArray(processPath);
            }

            // Get process memory info
            PROCESS_MEMORY_COUNTERS_EX pmc;
            if (GetProcessMemoryInfo(processHandle, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                // Convert bytes to MB and calculate as percentage of total memory
                MEMORYSTATUSEX memInfo;
                memInfo.dwLength = sizeof(MEMORYSTATUSEX);
                GlobalMemoryStatusEx(&memInfo);

                double memoryUsageMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
                double totalMemoryMB = memInfo.ullTotalPhys / (1024.0 * 1024.0);
                info.memoryUsage = static_cast<float>((memoryUsageMB / totalMemoryMB) * 100.0);
            }

            // Get process CPU info
            FILETIME createTime, exitTime, kernelTime, userTime;
            if (GetProcessTimes(processHandle, &createTime, &exitTime, &kernelTime, &userTime)) {
                ULARGE_INTEGER processTime;
                processTime.LowPart = kernelTime.dwLowDateTime + userTime.dwLowDateTime;
                processTime.HighPart = kernelTime.dwHighDateTime + userTime.dwHighDateTime;

                info.cpuUsage = static_cast<float>(calculateProcessCpuUsage(processHandle, processTime, sysTime));

                // ADD THESE LINES to validate CPU values:
                if (info.cpuUsage > 100.0f) {
                    LOG_WARNING(QString("Abnormal CPU usage detected for process %1: %2%, normalizing")
                               .arg(info.name).arg(info.cpuUsage));
                    info.cpuUsage = 100.0f;
                }
            }

            // Ensure process name isn't empty
            if (info.name.isEmpty()) {
                info.name = QString("Process-%1").arg(info.pid);
            }

            // Add to process list if it's using significant resources
            if (info.cpuUsage > 0.5f || info.memoryUsage > 0.5f) {
                m_processes.append(info);
            }

            CloseHandle(processHandle);
        }
    }

    // Sort by CPU usage (highest first)
    std::sort(m_processes.begin(), m_processes.end(),
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.cpuUsage > b.cpuUsage;
              });

    // Keep only top processes
    if (m_processes.size() > 20) {
        m_processes = m_processes.mid(0, 20);
    }
}

double SystemMonitorWin::calculateProcessCpuUsage(HANDLE processHandle, ULARGE_INTEGER &processTime, ULARGE_INTEGER &systemTime)
{
    DWORD processId = GetProcessId(processHandle);
    if (processId == 0) return 0.0;

    // First time measurement for the system
    if (m_lastSystemTime.QuadPart == 0) {
        m_lastProcessTimes[processId] = processTime;
        m_lastSystemTime = systemTime;
        return 0.0;
    }

    // First time seeing this process
    if (!m_lastProcessTimes.contains(processId)) {
        m_lastProcessTimes[processId] = processTime;
        return 0.0;
    }

    // Calculate deltas
    ULARGE_INTEGER lastProcTime = m_lastProcessTimes[processId];
    ULONGLONG processDelta = processTime.QuadPart - lastProcTime.QuadPart;
    ULONGLONG systemDelta = systemTime.QuadPart - m_lastSystemTime.QuadPart;

    // Guard against division by zero
    if (systemDelta == 0) {
        m_lastProcessTimes[processId] = processTime;
        m_lastSystemTime = systemTime;
        return 0.0;
    }

    // Get number of cores
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD numProcessors = sysInfo.dwNumberOfProcessors;

    // Calculate CPU usage as percentage and normalize by core count
    double cpuUsage = ((double)processDelta / (double)systemDelta) * 100.0;

    // Normalize by core count - a single-core process should max at 100%
    // even on multi-core systems
    if (cpuUsage > 100.0 * numProcessors) {
        LOG_DEBUG(QString("Abnormally high CPU usage for process %1: %2%")
                 .arg(processId).arg(cpuUsage));
        cpuUsage = 100.0 * numProcessors;
    }

    // Store current values for next calculation
    m_lastProcessTimes[processId] = processTime;
    m_lastSystemTime = systemTime;

    return cpuUsage;
}

