// Updated ConfigManager.h
#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QSettings>
#include <QMutex>

class APIManager;

class ConfigManager : public QObject
{
    Q_OBJECT
public:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager();

    bool initialize(APIManager* apiManager);

    // Getters
    QString serverUrl() const;
    int dataSendInterval() const;
    int idleTimeThreshold() const;
    QString machineId() const;
    bool trackKeyboardMouse() const;
    bool trackApplications() const;
    bool trackSystemMetrics() const;
    bool multiUserMode() const;
    QString defaultUsername() const;
    QString logLevel() const;
    QString logFilePath() const;

    // Setters
    void setServerUrl(const QString &url);
    void setDataSendInterval(int milliseconds);
    void setIdleTimeThreshold(int milliseconds);
    void setMachineId(const QString &id);
    void setTrackKeyboardMouse(bool track);
    void setTrackApplications(bool track);
    void setTrackSystemMetrics(bool track);
    void setMultiUserMode(bool enabled);
    void setDefaultUsername(const QString &username);
    void setLogLevel(const QString &level);
    void setLogFilePath(const QString &path);

    // Configuration operations
    bool loadLocalConfig();
    bool saveLocalConfig();

    // Server configuration (temporarily disabled)
    bool fetchServerConfig();
    bool updateConfigFromServer(const QJsonObject& serverConfig);

signals:
    void configChanged();
    void machineIdChanged(const QString& machineId);

private:
    // Helper methods
    void loadDefaults();
    QString configFilePath() const;
    bool configFileExists() const;

    // Configuration settings
    APIManager* m_apiManager;
    QSettings* m_settings;
    QMutex m_mutex;

    QString m_serverUrl;
    int m_dataSendInterval;
    int m_idleTimeThreshold;
    QString m_machineId;
    bool m_trackKeyboardMouse;
    bool m_trackApplications;
    bool m_trackSystemMetrics;
    bool m_multiUserMode;
    QString m_defaultUsername;
    QString m_logLevel;
    QString m_logFilePath;
    bool m_initialized;
};
#endif // CONFIGMANAGER_H