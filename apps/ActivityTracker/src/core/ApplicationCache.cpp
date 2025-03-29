#include "ApplicationCache.h"
#include "APIManager.h"
#include "logger/logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>

ApplicationCache::ApplicationCache(QObject *parent)
    : QObject(parent), m_apiManager(nullptr), m_initialized(false)
{
}

ApplicationCache::~ApplicationCache()
{
    // Save cache on destruction
    if (m_initialized) {
        saveCache();
    }
}

bool ApplicationCache::initialize(APIManager* apiManager)
{
    LOG_INFO("Initializing ApplicationCache");

    if (!apiManager) {
        LOG_ERROR("APIManager not provided for ApplicationCache");
        return false;
    }

    m_apiManager = apiManager;

    // Set cache path
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // Create directory if it doesn't exist
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_cachePath = appDataPath + "/app_cache.json";

    // Load existing cache
    loadCache();

    m_initialized = true;
    LOG_INFO("ApplicationCache initialized successfully");
    return true;
}

QString ApplicationCache::findAppId(const QString& appPath) const
{
    if (appPath.isEmpty()) {
        return QString();
    }

    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));

    // Normalize path for lookup
    QString normalizedPath = QDir::toNativeSeparators(appPath.toLower());

    return m_appIdsByPath.value(normalizedPath, QString());
}

QString ApplicationCache::registerApplication(const QString& appName, const QString& appPath)
{
    if (!m_initialized || !m_apiManager) {
        LOG_ERROR("ApplicationCache not initialized");
        return QString();
    }

    if (appName.isEmpty() || appPath.isEmpty()) {
        LOG_WARNING("Cannot register app with empty name or path");
        return QString();
    }

    LOG_INFO(QString("Registering application: %1 (%2)").arg(appName, appPath));

    // Check if already registered
    QString existingId = findAppId(appPath);
    if (!existingId.isEmpty()) {
        LOG_DEBUG(QString("Application already registered with ID: %1").arg(existingId));
        return existingId;
    }

    // Prepare detection data
    QJsonObject detectionData;
    detectionData["app_name"] = appName;
    detectionData["app_path"] = appPath;
    detectionData["tracking_enabled"] = true;

    // Call API to detect/register application
    QJsonObject responseData;
    bool success = m_apiManager->detectApplication(detectionData, responseData);

    if (success && responseData.contains("id")) {
        QString appId = responseData["id"].toString();

        LOG_INFO(QString("Application registered with ID: %1").arg(appId));

        // Create AppInfo object and store in cache
        AppInfo appInfo;
        appInfo.appId = appId;
        appInfo.appName = appName;
        appInfo.appPath = appPath;
        appInfo.isRestricted = responseData.value("is_restricted").toBool(false);
        appInfo.trackingEnabled = responseData.value("tracking_enabled").toBool(true);

        QMutexLocker locker(&m_mutex);
        m_appsById[appId] = appInfo;
        m_appIdsByPath[QDir::toNativeSeparators(appPath.toLower())] = appId;

        // Save cache to disk
        locker.unlock();
        saveCache();

        return appId;
    }

    LOG_ERROR("Failed to register application");
    return QString();
}

bool ApplicationCache::loadCache()
{
    QMutexLocker locker(&m_mutex);

    QFile file(m_cachePath);
    if (!file.exists()) {
        LOG_INFO("Application cache file doesn't exist yet, starting with empty cache");
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARNING(QString("Failed to open application cache file: %1").arg(file.errorString()));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        LOG_WARNING(QString("Failed to parse application cache file: %1").arg(error.errorString()));
        return false;
    }

    if (!doc.isObject()) {
        LOG_WARNING("Application cache file has invalid format (not a JSON object)");
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray apps = root["applications"].toArray();

    // Clear current cache
    m_appsById.clear();
    m_appIdsByPath.clear();

    // Load apps
    for (const QJsonValue& value : apps) {
        if (!value.isObject()) continue;

        QJsonObject appObj = value.toObject();

        AppInfo appInfo;
        appInfo.appId = appObj["id"].toString();
        appInfo.appName = appObj["name"].toString();
        appInfo.appPath = appObj["path"].toString();
        appInfo.appHash = appObj["hash"].toString();
        appInfo.isRestricted = appObj["is_restricted"].toBool(false);
        appInfo.trackingEnabled = appObj["tracking_enabled"].toBool(true);

        if (appInfo.appId.isEmpty() || appInfo.appPath.isEmpty()) {
            continue;
        }

        m_appsById[appInfo.appId] = appInfo;
        m_appIdsByPath[QDir::toNativeSeparators(appInfo.appPath.toLower())] = appInfo.appId;
    }

    LOG_INFO(QString("Loaded %1 applications from cache").arg(m_appsById.size()));
    return true;
}

bool ApplicationCache::saveCache()
{
    QMutexLocker locker(&m_mutex);

    QJsonObject root;
    QJsonArray apps;

    // Add all apps
    for (auto it = m_appsById.constBegin(); it != m_appsById.constEnd(); ++it) {
        const AppInfo& appInfo = it.value();

        QJsonObject appObj;
        appObj["id"] = appInfo.appId;
        appObj["name"] = appInfo.appName;
        appObj["path"] = appInfo.appPath;
        appObj["hash"] = appInfo.appHash;
        appObj["is_restricted"] = appInfo.isRestricted;
        appObj["tracking_enabled"] = appInfo.trackingEnabled;

        apps.append(appObj);
    }

    root["applications"] = apps;

    QJsonDocument doc(root);
    QByteArray data = doc.toJson(QJsonDocument::Indented);

    QFile file(m_cachePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOG_WARNING(QString("Failed to save application cache: %1").arg(file.errorString()));
        return false;
    }

    qint64 bytesWritten = file.write(data);
    file.close();

    if (bytesWritten != data.size()) {
        LOG_WARNING("Failed to write complete application cache data");
        return false;
    }

    LOG_INFO(QString("Saved %1 applications to cache").arg(apps.size()));
    return true;
}

void ApplicationCache::clear()
{
    QMutexLocker locker(&m_mutex);
    m_appsById.clear();
    m_appIdsByPath.clear();

    // Delete the cache file
    QFile file(m_cachePath);
    if (file.exists()) {
        file.remove();
    }

    LOG_INFO("Application cache cleared");
}