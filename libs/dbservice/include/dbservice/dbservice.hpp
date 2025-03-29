#pragma once
#include "dbservice/dbservice.h"
#include "logger/logger.h"
#include <QUuid>
#include <QSqlDriver>
#include <QElapsedTimer>

template<typename T>
DbService<T>::DbService(const DbConfig& config)
    : m_config(config)
{
    // Generate a unique connection name to avoid conflicts between multiple instances
    m_connectionName = QString("dbservice_%1_%2")
                       .arg(typeid(T).name())
                       .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    initializeDatabase(config);
}

template<typename T>
DbService<T>::~DbService() {
    // Close the database connection if open
    if (m_db.isOpen()) {
        LOG_DEBUG(QString("Closing database connection: %1").arg(m_connectionName));
        m_db.close();
    }

    // Remove the database connection
    QSqlDatabase::removeDatabase(m_connectionName);
}

template<typename T>
void DbService<T>::initializeDatabase(const DbConfig& config) {
    LOG_DEBUG(QString("Initializing database connection: %1").arg(m_connectionName));

    // Create a new connection with the unique name
    m_db = QSqlDatabase::addDatabase("QPSQL", m_connectionName);
    m_db.setHostName(config.host());
    m_db.setDatabaseName(config.database());
    m_db.setUserName(config.username());
    m_db.setPassword(config.password());
    m_db.setPort(config.port());

    // Set connection options
    m_db.setConnectOptions("application_name=DBService");

    // Open the connection
    if (!m_db.open()) {
        LOG_FATAL(QString("Database connection failed: %1 for database %2 on host %3 port %4")
                 .arg(m_db.lastError().text(), config.database(), config.host())
                 .arg(config.port()));
    } else {
        LOG_INFO(QString("Connected to database %1 on host %2 port %3 as user %4")
                .arg(config.database(), config.host())
                .arg(config.port())
                .arg(config.username()));
    }
}

template<typename T>
bool DbService<T>::ensureConnected() {
    if (!m_db.isOpen()) {
        LOG_WARNING("Database connection is closed, attempting to reopen...");

        if (m_db.open()) {
            LOG_INFO(QString("Successfully reopened database connection: %1").arg(m_connectionName));
            return true;
        } else {
            LOG_ERROR(QString("Failed to reopen database connection: %1").arg(m_db.lastError().text()));
            return false;
        }
    }
    return true;
}

template<typename T>
QList<T*> DbService<T>::executeSelectQuery(
    const QString& queryStr,
    const QMap<QString, QVariant>& params,
    const QueryProcessor& processor)
{
    if (!ensureConnected()) {
        LOG_ERROR("Cannot execute query, database is not connected");
        return QList<T*>();
    }

    QElapsedTimer timer;
    timer.start();

    QList<T*> results;  // Changed from QVector<T*> to QList<T*>

    try {
        // For queries without parameters, create a new connection to avoid prepared statement issues
        if (params.isEmpty()) {
            // Create a direct query without using prepare/bind
            QSqlQuery query(m_db);

            // Use exec(QString) instead of exec() to prevent prepare statement usage
            if (!query.exec(queryStr)) {
                LOG_ERROR(QString("Query failed: %1\nQuery: %2")
                         .arg(query.lastError().text(), queryStr));
                return QList<T*>();
            }

            // Process results
            while (query.next()) {
                results.append(processor(query));
            }

            LOG_DEBUG(QString("Query executed in %1 ms, returned %2 rows")
                     .arg(timer.elapsed())
                     .arg(results.size()));

            return results;
        } else {
            // For parameterized queries, use prepare/bind
            QSqlQuery query(m_db);
            if (!query.prepare(queryStr)) {
                LOG_ERROR(QString("Query preparation failed: %1\nQuery: %2")
                         .arg(query.lastError().text(), queryStr));
                return QList<T*>();
            }

            // Bind parameters
            for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
                query.bindValue(":" + it.key(), it.value());
            }

            if (!query.exec()) {
                LOG_ERROR(QString("Query failed: %1\nQuery: %2")
                         .arg(query.lastError().text(), queryStr));
                QMap<QString, QVariant> errorParams;
                for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
                    errorParams[it.key()] = it.value();
                }
                LOG_DATA(Logger::Error, errorParams);
                return QList<T*>();
            }

            // Process results
            while (query.next()) {
                results.append(processor(query));
            }

            LOG_DEBUG(QString("Query executed in %1 ms, returned %2 rows")
                     .arg(timer.elapsed())
                     .arg(results.size()));

            return results;
        }
    }
    catch (const std::exception& ex) {
        LOG_ERROR(QString("Exception during query execution: %1\nQuery: %2")
                 .arg(ex.what(), queryStr));
        return QList<T*>();
    }
}

template<typename T>
std::optional<T*> DbService<T>::executeSingleSelectQuery(
    const QString& queryStr,
    const QMap<QString, QVariant>& params,
    const QueryProcessor& processor)
{
    if (!ensureConnected()) {
        LOG_ERROR("Cannot execute query, database is not connected");
        return std::nullopt;
    }

    QElapsedTimer timer;
    timer.start();

    try {
        // For queries without parameters, use direct execution
        if (params.isEmpty()) {
            QSqlQuery query(m_db);

            // Use exec(QString) instead of exec() to prevent prepare statement usage
            if (!query.exec(queryStr)) {
                LOG_ERROR(QString("Query failed: %1\nQuery: %2")
                         .arg(query.lastError().text(), queryStr));
                return std::nullopt;
            }

            // Process the first result
            if (query.next()) {
                T* result = processor(query);
                LOG_DEBUG(QString("Query executed in %1 ms, returned 1 row")
                         .arg(timer.elapsed()));
                return result;
            }

            LOG_DEBUG(QString("Query executed in %1 ms, returned 0 rows")
                     .arg(timer.elapsed()));
            return std::nullopt;
        } else {
            // For parameterized queries, use prepare/bind
            QSqlQuery query(m_db);
            if (!query.prepare(queryStr)) {
                LOG_ERROR(QString("Query preparation failed: %1\nQuery: %2")
                         .arg(query.lastError().text(), queryStr));
                return std::nullopt;
            }

            // Bind parameters
            for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
                query.bindValue(":" + it.key(), it.value());
            }

            if (!query.exec()) {
                LOG_ERROR(QString("Query failed: %1\nQuery: %2")
                         .arg(query.lastError().text(), queryStr));
                QMap<QString, QVariant> errorParams;
                for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
                    errorParams[it.key()] = it.value();
                }
                LOG_DATA(Logger::Error, errorParams);
                return std::nullopt;
            }

            // Process the first result
            if (query.next()) {
                T* result = processor(query);
                LOG_DEBUG(QString("Query executed in %1 ms, returned 1 row")
                         .arg(timer.elapsed()));
                return result;
            }

            LOG_DEBUG(QString("Query executed in %1 ms, returned 0 rows")
                     .arg(timer.elapsed()));
            return std::nullopt;
        }
    }
    catch (const std::exception& ex) {
        LOG_ERROR(QString("Exception during query execution: %1\nQuery: %2")
                 .arg(ex.what(), queryStr));
        return std::nullopt;
    }
}

template<typename T>
bool DbService<T>::executeModificationQuery(
    const QString& queryStr,
    const QMap<QString, QVariant>& params)
{
    if (!ensureConnected()) {
        LOG_ERROR("Cannot execute query, database is not connected");
        return false;
    }

    QElapsedTimer timer;
    timer.start();

    try {
        // For queries without parameters, use direct execution
        if (params.isEmpty()) {
            QSqlQuery query(m_db);

            // Use exec(QString) instead of exec() to prevent prepare statement usage
            if (!query.exec(queryStr)) {
                LOG_ERROR(QString("Query failed: %1\nQuery: %2")
                         .arg(query.lastError().text(), queryStr));
                return false;
            }

            LOG_DEBUG(QString("Query executed in %1 ms, affected %2 rows")
                     .arg(timer.elapsed())
                     .arg(query.numRowsAffected()));
            return true;
        } else {
            // For parameterized queries, use prepare/bind
            QSqlQuery query(m_db);
            if (!query.prepare(queryStr)) {
                LOG_ERROR(QString("Query preparation failed: %1\nQuery: %2")
                         .arg(query.lastError().text(), queryStr));
                return false;
            }

            // Bind parameters
            for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
                query.bindValue(":" + it.key(), it.value());
            }

            if (!query.exec()) {
                LOG_ERROR(QString("Query failed: %1\nQuery: %2")
                         .arg(query.lastError().text(), queryStr));
                QMap<QString, QVariant> errorParams;
                for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
                    errorParams[it.key()] = it.value();
                }
                LOG_DATA(Logger::Error, errorParams);
                return false;
            }

            LOG_DEBUG(QString("Query executed in %1 ms, affected %2 rows")
                     .arg(timer.elapsed())
                     .arg(query.numRowsAffected()));
            return true;
        }
    }
    catch (const std::exception& ex) {
        LOG_ERROR(QString("Exception during query execution: %1\nQuery: %2")
                 .arg(ex.what(), queryStr));
        return false;
    }
}

template<typename T>
bool DbService<T>::beginTransaction() {
    if (!ensureConnected()) {
        LOG_ERROR("Cannot begin transaction, database is not connected");
        return false;
    }

    if (!m_db.driver()->hasFeature(QSqlDriver::Transactions)) {
        LOG_WARNING("Database driver does not support transactions");
        return false;
    }

    bool success = m_db.transaction();
    if (!success) {
        LOG_ERROR(QString("Failed to begin transaction: %1").arg(m_db.lastError().text()));
    } else {
        LOG_DEBUG("Transaction started");
    }

    return success;
}

template<typename T>
bool DbService<T>::commitTransaction() {
    if (!m_db.isOpen()) {
        LOG_ERROR("Cannot commit transaction, database is not connected");
        return false;
    }

    bool success = m_db.commit();
    if (!success) {
        LOG_ERROR(QString("Failed to commit transaction: %1").arg(m_db.lastError().text()));
    } else {
        LOG_DEBUG("Transaction committed");
    }

    return success;
}

template<typename T>
bool DbService<T>::rollbackTransaction() {
    if (!m_db.isOpen()) {
        LOG_ERROR("Cannot rollback transaction, database is not connected");
        return false;
    }

    bool success = m_db.rollback();
    if (!success) {
        LOG_ERROR(QString("Failed to rollback transaction: %1").arg(m_db.lastError().text()));
    } else {
        LOG_DEBUG("Transaction rolled back");
    }

    return success;
}

template<typename T>
bool DbService<T>::isConnectionValid() const {
    return m_db.isOpen() && m_db.isValid();
}

template<typename T>
QString DbService<T>::lastError() const {
    return m_db.lastError().text();
}

/**
 * @brief Create a query object for custom queries
 * @return QSqlQuery object connected to the database
 */
template<typename T>
QSqlQuery DbService<T>::createQuery() {
    if (!ensureConnected()) {
        LOG_ERROR("Cannot create query, database is not connected");
        return QSqlQuery();
    }

    return QSqlQuery(m_db);
}

