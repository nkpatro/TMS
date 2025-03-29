// MultiUserManager.cpp
#include "MultiUserManager.h"

#include <QJsonObject>

#include "core/APIManager.h"
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

QDateTime MultiUserManager::getLastActivityTime(const QString& username) const
{
    if (m_userLastActivity.contains(username)) {
        return m_userLastActivity[username];
    }
    return QDateTime();
}

void MultiUserManager::updateUserActivity(const QString& username)
{
    if (username.isEmpty()) {
        return;
    }

    // Update last activity time
    m_userLastActivity[username] = QDateTime::currentDateTime();

    // If the user isn't already added, add them
    if (!m_activeUsers.contains(username)) {
        m_activeUsers[username] = true;
        m_userStates[username] = UserSessionState::Active;
        emit userSessionChanged(username, true);
    }

    LOG_DEBUG(QString("Updated activity time for user '%1'").arg(username));
}

void MultiUserManager::processUserStateChange(const QString& username, UserSessionState newState)
{
    // Handle effects of state changes
    switch (newState) {
        case UserSessionState::Active:
            // If this user is becoming active and is not current user,
            // consider switching to this user
            if (!m_currentUser.isEmpty() && m_currentUser != username) {
                LOG_INFO(QString("User '%1' became active while '%2' was current")
                            .arg(username, m_currentUser));
            }
            break;

        case UserSessionState::Locked:
            // If the current user is being locked, we might need to pause tracking
            if (m_currentUser == username) {
                LOG_INFO(QString("Current user '%1' was locked").arg(username));
            }
            break;

        case UserSessionState::LoggedOut:
            // If the current user logged out, we need to clear their info and switch if possible
            if (m_currentUser == username) {
                m_userAuthTokens.remove(username);
                m_activeUsers.remove(username);
                m_currentUser.clear();

                // Find another active user to switch to
                for (auto it = m_activeUsers.constBegin(); it != m_activeUsers.constEnd(); ++it) {
                    if (it.value()) {
                        switchToUser(it.key());
                        break;
                    }
                }
            }
            break;

        default:
            break;
    }

    // Emit the state change signal
    emit userStateChanged(username, newState);
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
            // Update user state based on active status
            UserSessionState newState = active ? UserSessionState::Active : UserSessionState::Inactive;
            setUserState(user, newState);
            emit userSessionChanged(user, active);
        }
    }

    // Check for users that are no longer active (logged out)
    for (auto it = previousUsers.constBegin(); it != previousUsers.constEnd(); ++it) {
        const QString& user = it.key();
        bool wasActive = it.value();

        if (wasActive && (!m_activeUsers.contains(user) || !m_activeUsers[user])) {
            setUserState(user, UserSessionState::LoggedOut);
            emit userSessionChanged(user, false);
        }
    }

    // Check if current user changed
    if (m_currentUser != previousCurrentUser) {
        LOG_INFO(QString("Current user changed from %1 to %2")
                 .arg(previousCurrentUser, m_currentUser));

        // Don't clear auth token when switching users - we want to keep it
        // for when the user becomes active again
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

bool MultiUserManager::hasUserAuthToken(const QString& username) const
{
    return m_userAuthTokens.contains(username) && !m_userAuthTokens[username].isEmpty();
}

QString MultiUserManager::getUserAuthToken(const QString& username) const
{
    if (m_userAuthTokens.contains(username)) {
        return m_userAuthTokens[username];
    }
    return QString();
}

bool MultiUserManager::setUserAuthToken(const QString& username, const QString& token)
{
    if (username.isEmpty()) {
        LOG_WARNING("Attempted to set token for empty username");
        return false;
    }

    // Check if token is already set and hasn't changed - avoid redundant updates
    if (m_userAuthTokens.contains(username) && m_userAuthTokens[username] == token) {
        LOG_DEBUG(QString("Auth token for user '%1' unchanged, skipping update").arg(username));
        return true; // Token already set, no change needed
    }

    m_userAuthTokens[username] = token;

    // Add user to our tracking if not already there
    if (!m_activeUsers.contains(username)) {
        m_activeUsers[username] = true;
        m_userStates[username] = UserSessionState::Active;
        emit userSessionChanged(username, true);
    }

    LOG_INFO(QString("Auth token %1 for user '%2'")
             .arg(token.isEmpty() ? "cleared" : "set")
             .arg(username));

    return true;
}

UserSessionState MultiUserManager::getUserState(const QString& username) const
{
    if (m_userStates.contains(username)) {
        return m_userStates[username];
    }
    return UserSessionState::Inactive;
}

bool MultiUserManager::setUserState(const QString& username, UserSessionState state)
{
    LOG_INFO(QString("Setting user '%1' state to %2")
             .arg(username)
             .arg(static_cast<int>(state)));

    if (username.isEmpty()) {
        LOG_WARNING("Attempted to set state for empty username");
        return false;
    }

    // Update the user state
    m_userStates[username] = state;

    // Update active status based on state
    bool isActive = (state == UserSessionState::Active);
    bool wasActive = m_activeUsers.value(username, false);
    m_activeUsers[username] = isActive;

    // Emit signal if active status changed
    if (wasActive != isActive) {
        emit userSessionChanged(username, isActive);
    }

    return true;
}

bool MultiUserManager::authenticateUser(const QString& username, const QString& machineId, APIManager* apiManager)
{
    if (!apiManager) {
        LOG_ERROR("Cannot authenticate user: APIManager is null");
        return false;
    }

    if (username.isEmpty()) {
        LOG_ERROR("Cannot authenticate user: username is empty");
        return false;
    }

    // Check if we already have a valid token for this user
    if (hasUserAuthToken(username)) {
        // We have a token, set it in the APIManager
        QString token = getUserAuthToken(username);
        if (!token.isEmpty()) {
            LOG_INFO(QString("Using existing auth token for user '%1'").arg(username));
            apiManager->setAuthToken(token);
            return true;
        }
    }

    // We need to authenticate this user
    LOG_INFO(QString("Authenticating user '%1' on machine '%2'").arg(username, machineId));

    QJsonObject responseData;
    bool success = apiManager->authenticate(username, machineId, responseData);

    if (success && responseData.contains("token")) {
        // Save the authentication token
        QString token = responseData["token"].toString();
        setUserAuthToken(username, token);
        LOG_INFO(QString("Successfully authenticated user '%1'").arg(username));
        return true;
    }

    LOG_ERROR(QString("Failed to authenticate user '%1'").arg(username));
    return false;
}

bool MultiUserManager::switchToUser(const QString& username)
{
    if (username.isEmpty() || !m_activeUsers.contains(username)) {
        LOG_WARNING(QString("Cannot switch to invalid or inactive user: %1").arg(username));
        return false;
    }

    // First pause current user if different
    if (!m_currentUser.isEmpty() && m_currentUser != username) {
        pauseCurrentUser();
    }

    // Save previous user for logging
    QString previousUser = m_currentUser;

    // Set as current and update state
    m_currentUser = username;
    m_userStates[username] = UserSessionState::Active;
    m_activeUsers[username] = true;

    LOG_INFO(QString("Switched current user from '%1' to '%2'").arg(previousUser, username));

    // Emit signal only if user actually changed
    if (previousUser != username) {
        emit userSessionChanged(username, true);
    }

    return true;
}

bool MultiUserManager::pauseCurrentUser()
{
    if (m_currentUser.isEmpty()) {
        LOG_WARNING("No current user to pause");
        return false;
    }

    LOG_INFO(QString("Pausing current user: %1").arg(m_currentUser));

    // Set state to locked (paused)
    m_userStates[m_currentUser] = UserSessionState::Locked;

    // Keep in active users but mark as inactive
    m_activeUsers[m_currentUser] = false;

    // Emit signal
    emit userSessionChanged(m_currentUser, false);

    return true;
}

bool MultiUserManager::resumeUser(const QString& username)
{
    if (username.isEmpty() || !m_activeUsers.contains(username)) {
        LOG_WARNING(QString("Cannot resume invalid or unknown user: %1").arg(username));
        return false;
    }

    // If we're resuming a user different from current, switch to that user
    if (!m_currentUser.isEmpty() && m_currentUser != username) {
        return switchToUser(username);
    }

    // Otherwise just update the state
    m_userStates[username] = UserSessionState::Active;
    m_activeUsers[username] = true;

    LOG_INFO(QString("Resumed user: %1").arg(username));
    emit userSessionChanged(username, true);

    return true;
}