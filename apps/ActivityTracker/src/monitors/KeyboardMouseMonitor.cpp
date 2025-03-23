#include "KeyboardMouseMonitor.h"

KeyboardMouseMonitor::KeyboardMouseMonitor(QObject *parent)
    : QObject(parent)
    , m_idleTimeThreshold(300000)  // Default: 5 minutes
{
}

KeyboardMouseMonitor::~KeyboardMouseMonitor()
{
}

int KeyboardMouseMonitor::idleTimeThreshold() const
{
    return m_idleTimeThreshold;
}

void KeyboardMouseMonitor::setIdleTimeThreshold(int milliseconds)
{
    if (milliseconds >= 1000) {  // Minimum 1 second
        m_idleTimeThreshold = milliseconds;
    }
}