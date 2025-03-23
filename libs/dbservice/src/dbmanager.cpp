#include "dbservice/dbmanager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QDebug>
#include <QUuid>

DbManager& DbManager::instance() {
    static DbManager instance;
    return instance;
}

DbManager::DbManager() {
    LOG_DEBUG("DbManager instance created");
}

DbManager::~DbManager() {
    LOG_DEBUG("DbManager instance destroyed");

    // Close any open database connections
    for (const auto& connectionName : QSqlDatabase::connectionNames()) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        if (db.isOpen()) {
            LOG_DEBUG(QString("Closing database connection: %1").arg(connectionName));
            db.close();
        }
    }
}

bool DbManager::initialize(const DbConfig& config) {
    if (m_initialized) {
        LOG_WARNING("DbManager already initialized!");
        return true;
    }

    // Check if required drivers are available
    if (!checkDrivers()) {
        LOG_FATAL("Required database drivers are not available");
        return false;
    }

    m_config = config;

    // Test the database connection
    if (!testConnection()) {
        LOG_FATAL("Failed to connect to database during initialization");
        return false;
    }

    m_initialized = true;
    LOG_INFO(QString("DbManager successfully initialized for database %1 on host %2")
            .arg(config.database(), config.host()));
    return true;
}

bool DbManager::testConnection() const {
    // Create a unique connection name for testing
    QString testConnectionName = QString("test_connection_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    QSqlDatabase testDb;
    try {
        testDb = QSqlDatabase::addDatabase("QPSQL", testConnectionName);
        testDb.setHostName(m_config.host());
        testDb.setDatabaseName(m_config.database());
        testDb.setUserName(m_config.username());
        testDb.setPassword(m_config.password());
        testDb.setPort(m_config.port());

        if (!testDb.open()) {
            LOG_FATAL(QString("Test connection failed: %1").arg(testDb.lastError().text()));
            return false;
        }

        LOG_INFO(QString("Test connection successful to database %1 on host %2")
               .arg(m_config.database(), m_config.host()));

        // Execute a simple query to further validate the connection
        QSqlQuery query(testDb);
        if (!query.exec("SELECT 1")) {
            LOG_FATAL(QString("Test query failed: %1").arg(query.lastError().text()));
            testDb.close();
            QSqlDatabase::removeDatabase(testConnectionName);
            return false;
        }

        testDb.close();
    } catch (const std::exception& e) {
        LOG_FATAL(QString("Exception during test connection: %1").arg(e.what()));
        return false;
    }

    QSqlDatabase::removeDatabase(testConnectionName);
    return true;
}

bool DbManager::checkDrivers() const {
    QStringList drivers = QSqlDatabase::drivers();

    LOG_DEBUG(QString("Available SQL drivers: %1").arg(drivers.join(", ")));

    if (!drivers.contains("QPSQL")) {
        LOG_FATAL(QString("PostgreSQL driver (QPSQL) not available. Available drivers: %1")
                .arg(drivers.join(", ")));
        return false;
    }

    return true;
}