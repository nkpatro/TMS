// MultiUserManager.cpp
#include "MultiUserManager.h"
#include "logger/logger.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#include <WtsApi32.h>
#pragma comment(lib, "wtsapi32.lib")
#endif

MultiUserManager::MultiUserManager(QObject *parent)
    : QObject(parent)
    , m_isRunning(false)
    , m_previousUser("")
{
    // Set up polling timer
    connect(&m_pollTimer, &QTimer::timeout, this, &MultiUserManager::checkUserSessions);
    m_pollTimer.setInterval(5000); // Check every 5 seconds
}

MultiUserManager::~MultiUserManager()
{
    if (m_isRunning) {
        stop();
    }
}

bool MultiUserManager::initialize()
{
    LOG_INFO("Initializing MultiUserManager");

    // Don't set m_previousUser yet - we'll set it after initial update

    // Initial update of active users (with modified flag to prevent signal emission)
    bool initialUpdate = true;
    updateActiveUsers(initialUpdate);

    // Now that we have our initial state, set the previous user to match current
    m_previousUser = m_currentUser;

    return true;
}

bool MultiUserManager::start()
{
    if (m_isRunning) {
        LOG_WARNING("MultiUserManager is already running");
        return true;
    }

    LOG_INFO("Starting MultiUserManager");

    // Start polling timer
    m_pollTimer.start();
    m_isRunning = true;

    LOG_INFO("MultiUserManager started successfully");
    return true;
}

bool MultiUserManager::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("MultiUserManager is not running");
        return true;
    }

    LOG_INFO("Stopping MultiUserManager");

    // Stop polling timer
    m_pollTimer.stop();
    m_isRunning = false;

    LOG_INFO("MultiUserManager stopped successfully");
    return true;
}

QString MultiUserManager::currentUser() const
{
    return m_currentUser;
}

QStringList MultiUserManager::activeUsers() const
{
    QStringList users;
    for (auto it = m_activeUsers.constBegin(); it != m_activeUsers.constEnd(); ++it) {
        if (it.value()) {
            users.append(it.key());
        }
    }
    return users;
}

void MultiUserManager::checkUserSessions()
{
    QMap<QString, bool> previousUsers = m_activeUsers;
    QString previousCurrentUser = m_currentUser;

    // Update active users
    updateActiveUsers();

    // Check for changes in active users
    for (auto it = m_activeUsers.constBegin(); it != m_activeUsers.constEnd(); ++it) {
        const QString& user = it.key();
        bool active = it.value();

        // Check if user status changed
        if (!previousUsers.contains(user) || previousUsers[user] != active) {
            emit userSessionChanged(user, active);
        }
    }

    // Check for users that are no longer active
    for (auto it = previousUsers.constBegin(); it != previousUsers.constEnd(); ++it) {
        const QString& user = it.key();
        bool wasActive = it.value();

        if (wasActive && (!m_activeUsers.contains(user) || !m_activeUsers[user])) {
            emit userSessionChanged(user, false);
        }
    }

    // Check if current user changed
    if (m_currentUser != previousCurrentUser) {
        LOG_INFO(QString("Current user changed from %1 to %2")
                    .arg(previousCurrentUser, m_currentUser));
    }
}

void MultiUserManager::updateActiveUsers(bool initialUpdate)
{
    m_activeUsers.clear();

#ifdef Q_OS_WIN
    // Get active sessions on Windows
    WTS_SESSION_INFO* sessionInfo = nullptr;
    DWORD sessionCount = 0;

    if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessionInfo, &sessionCount)) {
        for (DWORD i = 0; i < sessionCount; i++) {
            if (sessionInfo[i].State == WTSActive) {
                LPWSTR userName = nullptr;
                DWORD userNameLen = 0;

                if (WTSQuerySessionInformationW(
                        WTS_CURRENT_SERVER_HANDLE,
                        sessionInfo[i].SessionId,
                        WTSUserName,
                        &userName,
                        &userNameLen) && userName) {

                    QString user = QString::fromWCharArray(userName);
                    if (!user.isEmpty()) {
                        m_activeUsers[user] = true;

                        // If this is the console session, mark as current user
                        if (sessionInfo[i].SessionId == WTSGetActiveConsoleSessionId()) {
                            m_currentUser = user;
                        }
                    }
                    WTSFreeMemory(userName);
                }
            }
        }
        WTSFreeMemory(sessionInfo);
    }
#else
    // On Unix-like systems, we could use utmp/utmpx
    // This is a simplified implementation
    QString userName = qgetenv("USER");
    if (userName.isEmpty()) {
        userName = qgetenv("USERNAME");
    }

    if (!userName.isEmpty()) {
        m_activeUsers[userName] = true;
        m_currentUser = userName;
    }
#endif

    // If we still don't have a current user but have active users,
    // use the first active user as the current user
    if (m_currentUser.isEmpty() && !m_activeUsers.isEmpty()) {
        m_currentUser = m_activeUsers.keys().first();
        LOG_INFO(QString("No user session marked as current, using first active user: %1").arg(m_currentUser));
    }

    // Log active users
    LOG_DEBUG(QString("Current user: %1").arg(m_currentUser));
    QStringList activeUsers = this->activeUsers();
    LOG_DEBUG(QString("Active users: %1").arg(activeUsers.join(", ")));

    // Emit signal if current user changed
    if (!initialUpdate && !m_currentUser.isEmpty() && m_previousUser != m_currentUser) {
        LOG_INFO(QString("Current user changed from '%1' to '%2'").arg(m_previousUser, m_currentUser));
        emit userSessionChanged(m_currentUser, true);
        m_previousUser = m_currentUser;
    }
}
