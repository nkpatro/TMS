#include "SystemMonitor.h"

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent)
    , m_highCpuThreshold(80.0f)  // Default: 80%
{
}

SystemMonitor::~SystemMonitor()
{
}

void SystemMonitor::setHighCpuThreshold(float percentage)
{
    if (percentage > 0.0f && percentage <= 100.0f) {
        m_highCpuThreshold = percentage;
    }
}

float SystemMonitor::highCpuThreshold() const
{
    return m_highCpuThreshold;
}