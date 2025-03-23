// SystemInfo.cpp
#include "SystemInfo.h"
#include <QHostInfo>
#include <QNetworkInterface>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QCryptographicHash>
#include <QSysInfo>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "ActivityEventModel.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <sysinfoapi.h>
#endif

#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <sys/sysinfo.h>
#endif

SystemInfo::SystemInfo(QObject *parent)
    : QObject(parent)
{
}

QString SystemInfo::getMachineHostName()
{
    return QHostInfo::localHostName();
}

QString SystemInfo::getMachineUniqueId()
{
    return QSysInfo::machineUniqueId();
}


QString SystemInfo::getMacAddress()
{
    QString macAddress;
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (QList<QNetworkInterface>::const_iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if ((it->flags() & QNetworkInterface::IsUp) &&
            !(it->flags() & QNetworkInterface::IsLoopBack))
        {
            macAddress = it->hardwareAddress();
            if (!macAddress.isEmpty())
            {
                break;
            }
        }
    }

    return macAddress;
}

QHostAddress SystemInfo::getLocalIPAddress()
{
    QList<QHostAddress> ipAddresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : ipAddresses) {
        if (address != QHostAddress::LocalHost && address.protocol() == QAbstractSocket::IPv4Protocol) {
            return address;
        }
    }

    return QHostAddress::LocalHost;
}

QString SystemInfo::getOperatingSystem()
{
    return QSysInfo::prettyProductName();
}

QString SystemInfo::getOSVersion()
{
    return QSysInfo::productVersion();
}

QString SystemInfo::getKernelVersion()
{
    return QSysInfo::kernelVersion();
}

QString SystemInfo::getCPUInfo()
{
    QString cpuInfo;

#ifdef Q_OS_WIN
    QProcess process;
    process.start("wmic cpu get Name, Manufacturer, NumberOfCores, MaxClockSpeed /value");
    process.waitForFinished();
    cpuInfo = QString::fromLocal8Bit(process.readAllStandardOutput());
    cpuInfo = cpuInfo.simplified();
#endif

#ifdef Q_OS_LINUX
    QFile file("/proc/cpuinfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        cpuInfo = stream.readAll();
        file.close();

        // Extract model name
        QRegExp rx("model name\\s*:\\s*(.+)");
        rx.setMinimal(true);
        if (rx.indexIn(cpuInfo) != -1) {
            cpuInfo = rx.cap(1);
        }
    }
#endif

#ifdef Q_OS_MACOS
    QProcess process;
    process.start("sysctl -n machdep.cpu.brand_string");
    process.waitForFinished();
    cpuInfo = QString::fromLocal8Bit(process.readAllStandardOutput());
    cpuInfo = cpuInfo.simplified();
#endif

    return cpuInfo;
}

QString SystemInfo::getGPUInfo()
{
    QString gpuInfo;

#ifdef Q_OS_WIN
    QProcess process;
    process.start("wmic path win32_VideoController get Name /value");
    process.waitForFinished();
    gpuInfo = QString::fromLocal8Bit(process.readAllStandardOutput());

    // Extract GPU name
    QRegularExpression rx("Name=(.+)");
    QRegularExpressionMatch match = rx.match(gpuInfo);
    if (match.hasMatch()) {
        gpuInfo = match.captured(1).simplified();
    }
#endif

#ifdef Q_OS_LINUX
    QProcess process;
    process.start("lspci | grep -i vga");
    process.waitForFinished();
    gpuInfo = QString::fromLocal8Bit(process.readAllStandardOutput());

    // Extract GPU name
    QRegExp rx("VGA compatible controller: (.+)");
    rx.setMinimal(true);
    if (rx.indexIn(gpuInfo) != -1) {
        gpuInfo = rx.cap(1).simplified();
    }
#endif

#ifdef Q_OS_MACOS
    QProcess process;
    process.start("system_profiler SPDisplaysDataType | grep Chipset");
    process.waitForFinished();
    gpuInfo = QString::fromLocal8Bit(process.readAllStandardOutput());

    // Extract GPU name
    QRegExp rx("Chipset Model: (.+)");
    rx.setMinimal(true);
    if (rx.indexIn(gpuInfo) != -1) {
        gpuInfo = rx.cap(1).simplified();
    }
#endif

    return gpuInfo;
}

int SystemInfo::getTotalRAMGB()
{
    int totalMemoryGB = 0;

#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    totalMemoryGB = memInfo.ullTotalPhys / (1024 * 1024 * 1024);
#endif

#ifdef Q_OS_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        totalMemoryGB = (info.totalram * info.mem_unit) / (1024 * 1024 * 1024);
    }
#endif

#ifdef Q_OS_MACOS
    QProcess process;
    process.start("sysctl -n hw.memsize");
    process.waitForFinished();
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    bool ok;
    quint64 totalMemoryBytes = output.trimmed().toULongLong(&ok);
    if (ok) {
        totalMemoryGB = totalMemoryBytes / (1024 * 1024 * 1024);
    }
#endif

    return totalMemoryGB;
}

double SystemInfo::getCurrentCPUUsage()
{
    static quint64 lastTotalUser = 0, lastTotalUserLow = 0, lastTotalSys = 0, lastTotalIdle = 0;
    quint64 totalUser, totalUserLow, totalSys, totalIdle;

    if (!readCPUStatistics(totalUser, totalUserLow, totalSys, totalIdle)) {
        return 0.0;
    }

    // Calculate CPU usage percentage
    if (lastTotalUser == 0 && lastTotalUserLow == 0 && lastTotalSys == 0 && lastTotalIdle == 0) {
        // First call, return 0
        lastTotalUser = totalUser;
        lastTotalUserLow = totalUserLow;
        lastTotalSys = totalSys;
        lastTotalIdle = totalIdle;
        return 0.0;
    }

    // Calculate the total CPU time
    quint64 totalUserTime = totalUser - lastTotalUser;
    quint64 totalUserLowTime = totalUserLow - lastTotalUserLow;
    quint64 totalSysTime = totalSys - lastTotalSys;
    quint64 totalIdleTime = totalIdle - lastTotalIdle;

    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;

    double total = totalUserTime + totalUserLowTime + totalSysTime + totalIdleTime;
    if (total == 0) {
        return 0.0;
    }

    // Calculate the percentage of non-idle time
    return 100.0 * (total - totalIdleTime) / total;
}

double SystemInfo::getCurrentMemoryUsage()
{
    double memoryUsagePercent = 0.0;

#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    memoryUsagePercent = memInfo.dwMemoryLoad;
#endif

#ifdef Q_OS_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        quint64 totalVirtualMem = info.totalram;
        totalVirtualMem += info.totalswap;
        totalVirtualMem *= info.mem_unit;

        quint64 usedVirtualMem = info.totalram - info.freeram;
        usedVirtualMem += info.totalswap - info.freeswap;
        usedVirtualMem *= info.mem_unit;

        memoryUsagePercent = 100.0 * usedVirtualMem / totalVirtualMem;
    }
#endif

#ifdef Q_OS_MACOS
    QProcess process;
    process.start("vm_stat");
    process.waitForFinished();
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());

    // Extract page size
    QRegExp rxPageSize("page size of (\\d+) bytes");
    rxPageSize.setMinimal(true);
    quint64 pageSize = 4096; // Default
    if (rxPageSize.indexIn(output) != -1) {
        bool ok;
        pageSize = rxPageSize.cap(1).toULongLong(&ok);
        if (!ok) pageSize = 4096;
    }

    // Extract free and total memory
    QRegExp rxFreePages("Pages free:\\s*(\\d+).");
    QRegExp rxActivePages("Pages active:\\s*(\\d+).");
    QRegExp rxInactivePages("Pages inactive:\\s*(\\d+).");
    QRegExp rxWiredPages("Pages wired down:\\s*(\\d+).");

    quint64 freePages = 0, activePages = 0, inactivePages = 0, wiredPages = 0;

    if (rxFreePages.indexIn(output) != -1) {
        bool ok;
        freePages = rxFreePages.cap(1).toULongLong(&ok);
        if (!ok) freePages = 0;
    }

    if (rxActivePages.indexIn(output) != -1) {
        bool ok;
        activePages = rxActivePages.cap(1).toULongLong(&ok);
        if (!ok) activePages = 0;
    }

    if (rxInactivePages.indexIn(output) != -1) {
        bool ok;
        inactivePages = rxInactivePages.cap(1).toULongLong(&ok);
        if (!ok) inactivePages = 0;
    }

    if (rxWiredPages.indexIn(output) != -1) {
        bool ok;
        wiredPages = rxWiredPages.cap(1).toULongLong(&ok);
        if (!ok) wiredPages = 0;
    }

    quint64 usedPages = activePages + wiredPages;
    quint64 totalPages = freePages + activePages + inactivePages + wiredPages;

    memoryUsagePercent = 100.0 * usedPages / totalPages;
#endif

    return memoryUsagePercent;
}

double SystemInfo::getCurrentGPUUsage()
{
    double gpuUsagePercent = 0.0;

    // GPU usage monitoring often requires vendor-specific libraries like NVIDIA NVML, AMD ADL, or Intel oneAPI
    // This is a placeholder - in a real app, you'd implement this with the appropriate library

    // For simplicity, we'll return a random value between 0 and 20 for demo purposes
    // In a real implementation, you would replace this with actual GPU monitoring
    gpuUsagePercent = QRandomGenerator::global()->bounded(20);

    return gpuUsagePercent;
}

QString SystemInfo::generateMachineFingerprint()
{
    // Combine various system identifiers to create a unique fingerprint
    QString fingerprint = getMachineHostName() +
                          getMachineUniqueId() +
                          getMacAddress() +
                          getOperatingSystem() +
                          getCPUInfo() +
                          QString::number(getTotalRAMGB());

    // Hash the fingerprint for consistency and privacy
    QByteArray hash = QCryptographicHash::hash(fingerprint.toUtf8(), QCryptographicHash::Sha256);

    // Return the first 32 chars of the hex representation as the fingerprint
    return hash.toHex().left(32);
}

QJsonObject SystemInfo::getAllSystemInfo()
{
    QJsonObject info;

    // System identification
    info["host_name"] = getMachineHostName();
    info["machine_id"] = getMachineUniqueId();
    info["mac_address"] = getMacAddress();
    info["ip_address"] = getLocalIPAddress().toString();
    info["fingerprint"] = generateMachineFingerprint();

    // Operating system
    info["os_name"] = getOperatingSystem();
    info["os_version"] = getOSVersion();
    info["kernel_version"] = getKernelVersion();

    // Hardware
    info["cpu_info"] = getCPUInfo();
    info["gpu_info"] = getGPUInfo();
    info["total_ram_gb"] = getTotalRAMGB();

    // Current metrics
    info["cpu_usage"] = getCurrentCPUUsage();
    info["memory_usage"] = getCurrentMemoryUsage();
    info["gpu_usage"] = getCurrentGPUUsage();

    return info;
}

QList<QPair<QString, QString>> SystemInfo::getCPUInfoPairs()
{
    QList<QPair<QString, QString>> infoPairs;

    // This is a simplified version - in a real app, you would parse /proc/cpuinfo on Linux
    // or query WMI on Windows or sysctl on macOS for detailed CPU information

    infoPairs.append(qMakePair(QString("Model"), getCPUInfo()));

    return infoPairs;
}

QList<QPair<QString, QString>> SystemInfo::getGPUInfoPairs()
{
    QList<QPair<QString, QString>> infoPairs;

    // This is a simplified version - in a real app, you would parse lspci on Linux
    // or query WMI on Windows or system_profiler on macOS for detailed GPU information

    infoPairs.append(qMakePair(QString("Model"), getGPUInfo()));

    return infoPairs;
}

bool SystemInfo::readCPUStatistics(quint64 &totalUser, quint64 &totalUserLow, quint64 &totalSys, quint64 &totalIdle)
{
    totalUser = totalUserLow = totalSys = totalIdle = 0;

#ifdef Q_OS_WIN
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime) == 0) {
        return false;
    }

    ULARGE_INTEGER idleTimeValue, kernelTimeValue, userTimeValue;
    idleTimeValue.LowPart = idleTime.dwLowDateTime;
    idleTimeValue.HighPart = idleTime.dwHighDateTime;
    kernelTimeValue.LowPart = kernelTime.dwLowDateTime;
    kernelTimeValue.HighPart = kernelTime.dwHighDateTime;
    userTimeValue.LowPart = userTime.dwLowDateTime;
    userTimeValue.HighPart = userTime.dwHighDateTime;

    totalIdle = idleTimeValue.QuadPart;
    totalSys = kernelTimeValue.QuadPart - idleTimeValue.QuadPart; // Kernel time includes idle time
    totalUser = userTimeValue.QuadPart;
    totalUserLow = 0; // Not used on Windows

    return true;
#endif

#ifdef Q_OS_LINUX
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    QString line = stream.readLine();
    file.close();

    if (line.startsWith("cpu ")) {
        QStringList parts = line.split(" ", QString::SkipEmptyParts);
        if (parts.size() >= 5) {
            totalUser = parts[1].toULongLong();
            totalUserLow = parts[2].toULongLong();
            totalSys = parts[3].toULongLong();
            totalIdle = parts[4].toULongLong();
            return true;
        }
    }

    return false;
#endif

#ifdef Q_OS_MACOS
    // On macOS, we would use host_statistics or host_statistics64 from the Mach kernel API
    // For simplicity, we'll just return false for now
    return false;
#endif

    // Fallback for unsupported platforms
    return false;
}