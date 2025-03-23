#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <functional>
#include <optional>
#include "dbconfig.h"
#include "logger/logger.h"

template<typename T>
class DbService {
public:
    using QueryProcessor = std::function<T*(const QSqlQuery&)>;

    explicit DbService(const DbConfig& config);
    ~DbService();

    // Execute a SELECT query and return multiple results
    QList<T*> executeSelectQuery(  // Changed from QVector to QList
        const QString& queryStr,
        const QMap<QString, QVariant>& params,
        const QueryProcessor& processor);

    // Execute a SELECT query and return single result
    std::optional<T*> executeSingleSelectQuery(
        const QString& queryStr,
        const QMap<QString, QVariant>& params,
        const QueryProcessor& processor);

    // Execute an INSERT, UPDATE, or DELETE query
    bool executeModificationQuery(
        const QString& queryStr,
        const QMap<QString, QVariant>& params);

    // Begin a transaction
    bool beginTransaction();

    // Commit a transaction
    bool commitTransaction();

    // Rollback a transaction
    bool rollbackTransaction();

    // Check if connection is valid
    bool isConnectionValid() const;

    // Get the last error
    QString lastError() const;

private:
    void initializeDatabase(const DbConfig& config);
    bool ensureConnected();
    QString m_connectionName;
    QSqlDatabase m_db;
    DbConfig m_config;
};