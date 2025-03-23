#include "ConfigManager.h"
#include "../core/APIManager.h"
#include "logger/logger.h"
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QHostInfo>
#include <QSysInfo>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_apiManager(nullptr)
    , m_settings(nullptr)
    , m_initialized(false)
{
    // Load defaults
    loadDefaults();
}

ConfigManager::~ConfigManager()
{
    delete m_settings;
}

bool ConfigManager::initialize(APIManager* apiManager)
{
    if (m_initialized) {
        LOG_WARNING("ConfigManager already initialized");
        return true;
    }

    LOG_INFO("Initializing ConfigManager");

    m_apiManager = apiManager;

    // Initialize settings object
    m_settings = new QSettings(configFilePath(), QSettings::IniFormat, this);

    m_initialized = true;
    return true;
}

QString ConfigManager::serverUrl() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_serverUrl;
}

int ConfigManager::dataSendInterval() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_dataSendInterval;
}

int ConfigManager::idleTimeThreshold() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_idleTimeThreshold;
}

QString ConfigManager::machineId() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_machineId;
}

bool ConfigManager::trackKeyboardMouse() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_trackKeyboardMouse;
}

bool ConfigManager::trackApplications() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_trackApplications;
}

bool ConfigManager::trackSystemMetrics() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_trackSystemMetrics;
}

bool ConfigManager::multiUserMode() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_multiUserMode;
}

QString ConfigManager::defaultUsername() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_defaultUsername;
}

QString ConfigManager::logLevel() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_logLevel;
}

QString ConfigManager::logFilePath() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_logFilePath;
}

void ConfigManager::setServerUrl(const QString &url)
{
    QMutexLocker locker(&m_mutex);
    if (m_serverUrl != url) {
        m_serverUrl = url;
        emit configChanged();
    }
}

void ConfigManager::setDataSendInterval(int milliseconds)
{
    QMutexLocker locker(&m_mutex);
    if (milliseconds >= 0 && m_dataSendInterval != milliseconds) {
        m_dataSendInterval = milliseconds;
        emit configChanged();
    }
}

void ConfigManager::setIdleTimeThreshold(int milliseconds)
{
    QMutexLocker locker(&m_mutex);
    if (milliseconds >= 1000 && m_idleTimeThreshold != milliseconds) {
        m_idleTimeThreshold = milliseconds;
        emit configChanged();
    }
}

void ConfigManager::setMachineId(const QString &id)
{
    QMutexLocker locker(&m_mutex);
    if (m_machineId != id) {
        m_machineId = id;
        emit machineIdChanged(id);
        emit configChanged();
    }
}

void ConfigManager::setTrackKeyboardMouse(bool track)
{
    QMutexLocker locker(&m_mutex);
    if (m_trackKeyboardMouse != track) {
        m_trackKeyboardMouse = track;
        emit configChanged();
    }
}

void ConfigManager::setTrackApplications(bool track)
{
    QMutexLocker locker(&m_mutex);
    if (m_trackApplications != track) {
        m_trackApplications = track;
        emit configChanged();
    }
}

void ConfigManager::setTrackSystemMetrics(bool track)
{
    QMutexLocker locker(&m_mutex);
    if (m_trackSystemMetrics != track) {
        m_trackSystemMetrics = track;
        emit configChanged();
    }
}

void ConfigManager::setMultiUserMode(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    if (m_multiUserMode != enabled) {
        m_multiUserMode = enabled;
        emit configChanged();
    }
}

void ConfigManager::setDefaultUsername(const QString &username)
{
    QMutexLocker locker(&m_mutex);
    if (m_defaultUsername != username) {
        m_defaultUsername = username;
        emit configChanged();
    }
}

void ConfigManager::setLogLevel(const QString &level)
{
    QMutexLocker locker(&m_mutex);
    if (m_logLevel != level) {
        m_logLevel = level;
        emit configChanged();
    }
}

void ConfigManager::setLogFilePath(const QString &path)
{
    QMutexLocker locker(&m_mutex);
    if (m_logFilePath != path) {
        m_logFilePath = path;
        emit configChanged();
    }
}

bool ConfigManager::loadLocalConfig()
{
    LOG_INFO("Loading local configuration");

    if (!m_initialized) {
        LOG_ERROR("ConfigManager not initialized");
        return false;
    }

    if (!m_settings) {
        LOG_ERROR("Settings object not initialized");
        return false;
    }

    QMutexLocker locker(&m_mutex);

    // First check if config file exists
    if (!configFileExists()) {
        LOG_INFO("Configuration file not found, using defaults");
        return true;
    }

    // Load settings from file
    m_serverUrl = m_settings->value("ServerUrl", m_serverUrl).toString();
    m_dataSendInterval = m_settings->value("DataSendInterval", m_dataSendInterval).toInt();
    m_idleTimeThreshold = m_settings->value("IdleTimeThreshold", m_idleTimeThreshold).toInt();
    m_machineId = m_settings->value("MachineId", m_machineId).toString();
    m_trackKeyboardMouse = m_settings->value("TrackKeyboardMouse", m_trackKeyboardMouse).toBool();
    m_trackApplications = m_settings->value("TrackApplications", m_trackApplications).toBool();
    m_trackSystemMetrics = m_settings->value("TrackSystemMetrics", m_trackSystemMetrics).toBool();
    m_multiUserMode = m_settings->value("MultiUserMode", m_multiUserMode).toBool();
    m_defaultUsername = m_settings->value("DefaultUsername", m_defaultUsername).toString();
    m_logLevel = m_settings->value("LogLevel", m_logLevel).toString();
    m_logFilePath = m_settings->value("LogFilePath", m_logFilePath).toString();

    // Validate and correct settings
    if (m_dataSendInterval < 0) {
        m_dataSendInterval = 0; // 0 means immediate sending
    }

    if (m_idleTimeThreshold < 1000) {
        m_idleTimeThreshold = 60000; // Minimum 1 minute
    }

    // Apply log settings
    if (m_logLevel == "debug") {
        Logger::instance()->setLogLevel(Logger::Debug);
    } else if (m_logLevel == "info") {
        Logger::instance()->setLogLevel(Logger::Info);
    } else if (m_logLevel == "warning") {
        Logger::instance()->setLogLevel(Logger::Warning);
    } else if (m_logLevel == "error") {
        Logger::instance()->setLogLevel(Logger::Error);
    }

    if (!m_logFilePath.isEmpty()) {
        Logger::instance()->setLogFile(m_logFilePath);
    }

    LOG_INFO("Local configuration loaded successfully");
    return true;
}

bool ConfigManager::saveLocalConfig()
{
    LOG_INFO("Saving configuration to: " + configFilePath());

    if (!m_initialized) {
        LOG_ERROR("ConfigManager not initialized");
        return false;
    }

    if (!m_settings) {
        LOG_ERROR("Settings object not initialized");
        return false;
    }

    QMutexLocker locker(&m_mutex);

    // Save settings to file
    m_settings->setValue("ServerUrl", m_serverUrl);
    m_settings->setValue("DataSendInterval", m_dataSendInterval);
    m_settings->setValue("IdleTimeThreshold", m_idleTimeThreshold);
    m_settings->setValue("MachineId", m_machineId);
    m_settings->setValue("TrackKeyboardMouse", m_trackKeyboardMouse);
    m_settings->setValue("TrackApplications", m_trackApplications);
    m_settings->setValue("TrackSystemMetrics", m_trackSystemMetrics);
    m_settings->setValue("MultiUserMode", m_multiUserMode);
    m_settings->setValue("DefaultUsername", m_defaultUsername);
    m_settings->setValue("LogLevel", m_logLevel);
    m_settings->setValue("LogFilePath", m_logFilePath);
    m_settings->sync();

    LOG_INFO("Configuration saved successfully");
    return (m_settings->status() == QSettings::NoError);
}

bool ConfigManager::fetchServerConfig()
{
    if (!m_initialized) {
        LOG_ERROR("ConfigManager not initialized");
        return false;
    }

    if (!m_apiManager) {
        LOG_ERROR("API Manager not initialized");
        return false;
    }

    LOG_INFO("Fetching configuration from server");

    QJsonObject serverConfig;
    if (!m_apiManager->getServerConfiguration(serverConfig)) {
        LOG_ERROR("Failed to fetch server configuration");
        return false;
    }

    return updateConfigFromServer(serverConfig);
}

bool ConfigManager::updateConfigFromServer(const QJsonObject& serverConfig)
{
    LOG_INFO("Updating configuration from server");

    QMutexLocker locker(&m_mutex);

    // Update settings from server JSON
    if (serverConfig.contains("ServerUrl"))
        m_serverUrl = serverConfig["ServerUrl"].toString();

    if (serverConfig.contains("DataSendInterval"))
        m_dataSendInterval = serverConfig["DataSendInterval"].toInt();

    if (serverConfig.contains("IdleTimeThreshold"))
        m_idleTimeThreshold = serverConfig["IdleTimeThreshold"].toInt();

    if (serverConfig.contains("TrackKeyboardMouse"))
        m_trackKeyboardMouse = serverConfig["TrackKeyboardMouse"].toBool();

    if (serverConfig.contains("TrackApplications"))
        m_trackApplications = serverConfig["TrackApplications"].toBool();

    if (serverConfig.contains("TrackSystemMetrics"))
        m_trackSystemMetrics = serverConfig["TrackSystemMetrics"].toBool();

    if (serverConfig.contains("MultiUserMode"))
        m_multiUserMode = serverConfig["MultiUserMode"].toBool();

    if (serverConfig.contains("LogLevel"))
        m_logLevel = serverConfig["LogLevel"].toString();

    // Save updated config to file
    locker.unlock();
    bool success = saveLocalConfig();

    // Signal that configuration has changed
    if (success) {
        emit configChanged();
    }

    return success;
}

void ConfigManager::loadDefaults()
{
    // Default configuration values
    m_serverUrl = "http://localhost:8080";
    m_dataSendInterval = 60000; // 1 minute
    m_idleTimeThreshold = 300000; // 5 minutes
    m_machineId = "";
    m_trackKeyboardMouse = true;
    m_trackApplications = true;
    m_trackSystemMetrics = true;
    m_multiUserMode = false;
    m_defaultUsername = "";
    m_logLevel = "info";
    m_logFilePath = "";
}

QString ConfigManager::configFilePath() const
{
    // Determine config file path based on platform
#ifdef Q_OS_WIN
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString configDir = "/etc/activity_tracker";
#endif

    // Ensure directory exists
    QDir dir(configDir);
    if (!dir.exists()) {
        dir.mkpath(configDir);
    }

    return configDir + "/activity_tracker.conf";
}

bool ConfigManager::configFileExists() const
{
    return QFile::exists(configFilePath());
}