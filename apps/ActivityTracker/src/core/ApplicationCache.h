#ifndef APPLICATIONCACHE_H
#define APPLICATIONCACHE_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QJsonObject>
#include <QMutex>
#include <QDir>

class APIManager;

class ApplicationCache : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationCache(QObject *parent = nullptr);
    ~ApplicationCache();

    struct AppInfo {
        QString appId;
        QString appName;
        QString appPath;
        QString appHash;
        bool isRestricted;
        bool trackingEnabled;
    };

    bool initialize(APIManager* apiManager);
    QString findAppId(const QString& appPath) const;
    QString registerApplication(const QString& appName, const QString& appPath);
    bool loadCache();
    bool saveCache();
    void clear();

private:
    APIManager* m_apiManager;
    QString m_cachePath;
    QMap<QString, AppInfo> m_appsById;
    QMap<QString, QString> m_appIdsByPath;
    QMutex m_mutex;
    bool m_initialized;
};

#endif // APPLICATIONCACHE_H