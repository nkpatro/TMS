// Updated ConfigManager.cpp
#include "ConfigManager.h"
#include "../core/APIManager.h"
#include "logger/logger.h"
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QHostInfo>
#include <QSysInfo>
#include <QJsonDocument>
#include <QFileInfo>

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

    // Get the config file path
    QString configPath = configFilePath();
    LOG_INFO("Config file path: " + configPath);

    // Ensure the directory exists
    QFileInfo fileInfo(configPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        LOG_INFO("Creating config directory: " + dir.path());
        if (!dir.mkpath(".")) {
            LOG_ERROR("Failed to create config directory");
            return false;
        }
    }

    // Initialize settings object
    m_settings = new QSettings(configPath, QSettings::IniFormat, this);

    if (m_settings->status() != QSettings::NoError) {
        LOG_ERROR("Error initializing QSettings: " + QString::number(m_settings->status()));
        return false;
    }

    m_initialized = true;
    return true;
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

    // First check if config file exists
    bool fileExists = configFileExists();

    // Log debug info about the config file
    if (fileExists) {
        LOG_INFO("Configuration file found: " + m_settings->fileName());
        LOG_DEBUG("Config contains " + QString::number(m_settings->allKeys().size()) + " keys");
    } else {
        LOG_INFO("Configuration file not found, will use defaults");
        // Create a default config file
        return saveLocalConfig();
    }

    // Loading configuration values with QMutexLocker scope
    {
        QMutexLocker locker(&m_mutex);

        // Define explicit defaults to make it clear what's happening when loading
        const QString DEFAULT_SERVER_URL = "http://localhost:8080";
        const int DEFAULT_DATA_SEND_INTERVAL = 60000;
        const int DEFAULT_IDLE_TIME_THRESHOLD = 300000;
        const bool DEFAULT_TRACK_KBM = true;
        const bool DEFAULT_TRACK_APPS = true;
        const bool DEFAULT_TRACK_METRICS = true;
        const bool DEFAULT_MULTI_USER = false;
        const QString DEFAULT_LOG_LEVEL = "info";

        // Load settings from file with explicit default values
        m_serverUrl = m_settings->value("ServerUrl", DEFAULT_SERVER_URL).toString();
        m_dataSendInterval = m_settings->value("DataSendInterval", DEFAULT_DATA_SEND_INTERVAL).toInt();
        m_idleTimeThreshold = m_settings->value("IdleTimeThreshold", DEFAULT_IDLE_TIME_THRESHOLD).toInt();
        m_machineId = m_settings->value("MachineId", m_machineId).toString();
        m_trackKeyboardMouse = m_settings->value("TrackKeyboardMouse", DEFAULT_TRACK_KBM).toBool();
        m_trackApplications = m_settings->value("TrackApplications", DEFAULT_TRACK_APPS).toBool();
        m_trackSystemMetrics = m_settings->value("TrackSystemMetrics", DEFAULT_TRACK_METRICS).toBool();
        m_multiUserMode = m_settings->value("MultiUserMode", DEFAULT_MULTI_USER).toBool();
        m_defaultUsername = m_settings->value("DefaultUsername", m_defaultUsername).toString();
        m_logLevel = m_settings->value("LogLevel", DEFAULT_LOG_LEVEL).toString();
        m_logFilePath = m_settings->value("LogFilePath", m_logFilePath).toString();

        // Validate and correct settings
        if (m_dataSendInterval < 0) {
            LOG_WARNING("Invalid DataSendInterval corrected from " + QString::number(m_dataSendInterval) + " to 0");
            m_dataSendInterval = 0; // 0 means immediate sending
        }

        if (m_idleTimeThreshold < 1000) {
            LOG_WARNING("Invalid IdleTimeThreshold corrected from " + QString::number(m_idleTimeThreshold) + " to 60000");
            m_idleTimeThreshold = 60000; // Minimum 1 minute
        }

        // Auto-generate machine ID if not set
        if (m_machineId.isEmpty()) {
            QString hostname = QHostInfo::localHostName();
            QString machineUniqueId = QSysInfo::machineUniqueId();

            if (machineUniqueId.isEmpty()) {
                // Fallback if QSysInfo::machineUniqueId() returns empty
                machineUniqueId = QSysInfo::machineHostName() + "-" +
                                  QSysInfo::productType() + "-" +
                                  QSysInfo::productVersion();
            }

            m_machineId = hostname + "-" + machineUniqueId;
            LOG_INFO("Generated new machine ID: " + m_machineId);
            m_settings->setValue("MachineId", m_machineId);
            m_settings->sync();
        }
    } // QMutexLocker released here

    // Apply log settings inline
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

    // Ensure settings are written to disk
    m_settings->sync();

    QSettings::Status status = m_settings->status();
    if (status != QSettings::NoError) {
        LOG_ERROR("Failed to save configuration, error code: " + QString::number(status));
        return false;
    }

    LOG_INFO("Configuration saved successfully");
    return true;
}

bool ConfigManager::fetchServerConfig()
{
    // Removed functionality for now
    LOG_INFO("Server configuration functionality temporarily disabled");
    return true;
}

bool ConfigManager::updateConfigFromServer(const QJsonObject& serverConfig)
{
    // Removed functionality for now
    LOG_INFO("Server configuration functionality temporarily disabled");
    return true;
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

    return configDir + "/activity_tracker.conf";
}

bool ConfigManager::configFileExists() const
{
    if (!m_settings) {
        return false;
    }

    QString path = m_settings->fileName();
    bool exists = QFile::exists(path);

    if (exists) {
        // Check if the file has content
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            bool hasContent = file.size() > 0;
            file.close();
            return hasContent;
        }
    }

    return false;
}

// Getter implementations remain the same
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

// Setter implementations remain the same
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