#pragma once
#include <QString>

class DbConfig {
public:
    static DbConfig fromResource();
    static DbConfig fromEnvironment();
    static DbConfig fromFile(const QString& configPath);

    QString host() const { return m_host; }
    QString database() const { return m_database; }
    QString username() const { return m_username; }
    QString password() const { return m_password; }
    int port() const { return m_port; }

private:
    QString m_host;
    QString m_database;
    QString m_username;
    QString m_password;
    int m_port;
};