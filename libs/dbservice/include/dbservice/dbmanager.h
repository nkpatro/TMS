#pragma once
#include "dbservice.hpp"
#include "dbconfig.h"
#include "logger/logger.h"
#include <memory>
#include <typeindex>

class DbManager {
public:
    // Get singleton instance
    static DbManager& instance();

    // Initialize with database configuration
    bool initialize(const DbConfig& config);

    // Check if database manager is initialized
    bool isInitialized() const { return m_initialized; }

    // Get database configuration
    const DbConfig& config() const { return m_config; }

    // Test database connection
    bool testConnection() const;

    // Get service for a specific model
    template<typename T>
    DbService<T>& getService() {
        if (!m_initialized) {
            LOG_FATAL(QString("DbManager not initialized! Attempting to get service for %1")
                     .arg(typeid(T).name()));
            // Throw an exception instead of calling qFatal to allow proper error handling
            throw std::runtime_error("DbManager not initialized");
        }

        const std::type_index typeIndex(typeid(T));
        if (!m_services.contains(typeIndex)) {
            LOG_DEBUG(QString("Creating new DB service for %1").arg(typeid(T).name()));
            m_services[typeIndex] = std::make_shared<DbService<T>>(m_config);
        }

        return *std::static_pointer_cast<DbService<T>>(m_services[typeIndex]);
    }

private:
    // Private constructor and destructor for singleton
    DbManager();
    ~DbManager();

    // Delete copy constructor and assignment operator
    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

    // Check if drivers are available
    bool checkDrivers() const;

    // Member variables
    bool m_initialized = false;
    DbConfig m_config;
    QMap<std::type_index, std::shared_ptr<void>> m_services;
};