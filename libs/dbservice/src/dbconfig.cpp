#include "dbservice/dbconfig.h"
#include <QSettings>
#include <QProcessEnvironment>
#include <QFile>
#include <QDebug>

DbConfig DbConfig::fromResource() {
    if (!QFile::exists(":/config/database.ini")) {
        qDebug() << "Database configuration file not found in resources!";
        return DbConfig::fromEnvironment();
    }
    return DbConfig::fromFile(":/config/database.ini");
}

DbConfig DbConfig::fromEnvironment() {
    DbConfig config;
    auto env = QProcessEnvironment::systemEnvironment();

    config.m_host = env.value("DB_HOST", "10.1.71.113");
    config.m_database = env.value("DB_NAME", "testdb03");
    config.m_username = env.value("DB_USER", "postgres");
    config.m_password = env.value("DB_PASSWORD", "logics22");
    config.m_port = env.value("DB_PORT", "5432").toInt();

    return config;
}

DbConfig DbConfig::fromFile(const QString& configPath) {
    DbConfig config;
    QSettings settings(configPath, QSettings::IniFormat);

    settings.beginGroup("Database");
    config.m_host = settings.value("host", "localhost").toString();
    config.m_database = settings.value("database", "postgres").toString();
    config.m_username = settings.value("username", "postgres").toString();
    config.m_password = settings.value("password", "").toString();
    config.m_port = settings.value("port", 5432).toInt();
    settings.endGroup();

    return config;
}