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
    QList<T*> executeSelectQuery(
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

    // Execute an INSERT query with a RETURNING clause and get the returned ID
    bool executeInsertWithReturningId(
        const QString& query,
        const QMap<QString, QVariant>& params,
        const QString& idColumnName,
        std::function<void(const QVariant&)> idHandler);

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

    // Create a query object for custom queries
    QSqlQuery createQuery();

private:
    void initializeDatabase(const DbConfig& config);
    bool ensureConnected();
    QString m_connectionName;
    QSqlDatabase m_db;
    DbConfig m_config;
};