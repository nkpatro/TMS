#ifndef BASEREPOSITORY_H
#define BASEREPOSITORY_H

#include <QObject>
#include <QSharedPointer>
#include <QList>
#include <QUuid>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include <QJsonDocument>

#include "dbservice/dbservice.hpp"
#include "logger/logger.h"
#include "Core/ModelFactory.h"

/**
 * @brief Base template class for all repositories
 *
 * This class provides common database access functionality for repositories
 * to reduce code duplication. It integrates with ModelFactory for model creation
 * and provides standardized CRUD operations.
 */
template <typename T>
class BaseRepository : public QObject {
public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit BaseRepository(QObject* parent = nullptr)
        : QObject(parent), m_dbService(nullptr), m_initialized(false)
    {
        LOG_DEBUG(QString("%1 repository instance created").arg(getEntityName()));
    }

    /**
     * @brief Destructor
     */
    virtual ~BaseRepository()
    {
        LOG_DEBUG(QString("%1 repository instance destroyed").arg(getEntityName()));
    }

    /**
     * @brief Initialize the repository with a database service
     * @param dbService Database service for the model type
     * @return True if initialization was successful
     */
    bool initialize(DbService<T>* dbService) {
        if (m_initialized) {
            LOG_WARNING(QString("%1 repository already initialized").arg(getEntityName()));
            return true;
        }

        if (!dbService) {
            LOG_ERROR(QString("Cannot initialize %1 repository: null database service").arg(getEntityName()));
            return false;
        }

        m_dbService = dbService;
        m_initialized = true;
        LOG_INFO(QString("%1 repository initialized successfully").arg(getEntityName()));
        return true;
    }

    /**
     * @brief Check if the repository is initialized
     * @return True if initialized, false otherwise
     */
    bool isInitialized() const {
        return m_initialized;
    }

    /**
     * @brief Get database service last error
     * @return Last error string
     */
    QString lastError() const {
        if (!m_dbService) {
            return "Database service not initialized";
        }
        return m_dbService->lastError();
    }

    /**
     * @brief Save a model to the database
     * @param model The model to save
     * @return True if save was successful
     */
    virtual bool save(T* model) {
        if (!ensureInitialized()) {
            return false;
        }

        // Validate model before saving
        QStringList validationErrors;
        if (!validateModel(model, validationErrors)) {
            LOG_ERROR(QString("Cannot save %1: validation failed - %2")
                     .arg(getEntityName(), validationErrors.join(", ")));
            return false;
        }

        // Check if this is a new record (null ID) or an existing one with ID
        bool isNewRecord = model->id().isNull();

        // Prepare query parameters
        QMap<QString, QVariant> params = prepareParamsForSave(model);

        // Build query - if new record and ID is null, we'll use RETURNING to get the generated ID
        QString query;
        if (isNewRecord) {
            query = buildSaveQueryWithReturning();
        } else {
            query = buildSaveQuery();
        }

        // For new records with null IDs, we expect the DB to generate the ID
        if (isNewRecord && model->id().isNull()) {
            // Execute query that returns the generated ID
            QUuid generatedId;
            bool success = m_dbService->executeInsertWithReturningId(
                query,
                params,
                getIdParamName(), // Use existing method to get column name
                [&generatedId](const QVariant& value) {
                    generatedId = QUuid(value.toString());
                }
            );

            if (success) {
                // Update the model with the generated ID
                if (!generatedId.isNull()) {
                    model->setId(generatedId);
                    LOG_INFO(QString("%1 saved successfully with database-generated ID: %2")
                            .arg(getEntityName(), generatedId.toString()));
                } else {
                    LOG_WARNING(QString("%1 saved but failed to retrieve generated ID")
                               .arg(getEntityName()));
                }
            } else {
                LOG_ERROR(QString("Failed to save %1: %2")
                         .arg(getEntityName(), m_dbService->lastError()));
            }

            return success;
        } else {
            // For existing records or when ID is already set, use standard insert
            bool success = m_dbService->executeModificationQuery(query, params);

            if (success) {
                LOG_INFO(QString("%1 saved successfully with ID: %2")
                        .arg(getEntityName(), getModelId(model)));
            } else {
                LOG_ERROR(QString("Failed to save %1: %2 - %3")
                         .arg(getEntityName(), getModelId(model), m_dbService->lastError()));
            }

            return success;
        }
    }

    /**
     * @brief Update a model in the database
     * @param model The model to update
     * @return True if update was successful
     */
    virtual bool update(T* model) {
        if (!ensureInitialized()) {
            return false;
        }

        // Validate model before updating
        QStringList validationErrors;
        if (!validateModel(model, validationErrors)) {
            LOG_ERROR(QString("Cannot update %1: validation failed - %2")
                     .arg(getEntityName(), validationErrors.join(", ")));
            return false;
        }

        // Prepare query parameters
        QMap<QString, QVariant> params = prepareParamsForUpdate(model);

        // Build query
        QString query = buildUpdateQuery();

        // Execute query
        bool success = m_dbService->executeModificationQuery(query, params);

        if (success) {
            LOG_INFO(QString("%1 updated successfully: %2").arg(getEntityName(), getModelId(model)));
        } else {
            LOG_ERROR(QString("Failed to update %1: %2 - %3")
                     .arg(getEntityName(), getModelId(model), m_dbService->lastError()));
        }

        return success;
    }

    /**
     * @brief Get a model by its ID
     * @param id The model ID
     * @return Shared pointer to the model or nullptr if not found
     */
    virtual QSharedPointer<T> getById(const QUuid& id) {
        if (!ensureInitialized()) {
            return nullptr;
        }

        QMap<QString, QVariant> params;
        params[getIdParamName()] = id.toString(QUuid::WithoutBraces);

        QString query = buildGetByIdQuery();

        auto result = m_dbService->executeSingleSelectQuery(
            query,
            params,
            [this](const QSqlQuery& query) -> T* {
                return createModelFromQuery(query);
            }
        );

        if (result) {
            LOG_DEBUG(QString("%1 found with ID: %2").arg(getEntityName(), id.toString()));
            return QSharedPointer<T>(*result);
        }

        LOG_DEBUG(QString("%1 not found with ID: %2").arg(getEntityName(), id.toString()));
        return nullptr;
    }

    /**
     * @brief Get all models
     * @return List of models
     */
    virtual QList<QSharedPointer<T>> getAll() {
        if (!ensureInitialized()) {
            return QList<QSharedPointer<T>>();
        }

        QString query = buildGetAllQuery();

        auto models = m_dbService->executeSelectQuery(
            query,
            QMap<QString, QVariant>(),
            [this](const QSqlQuery& query) -> T* {
                return createModelFromQuery(query);
            }
        );

        QList<QSharedPointer<T>> result;
        for (auto model : models) {
            result.append(QSharedPointer<T>(model));
        }

        LOG_INFO(QString("Retrieved %1 %2 records").arg(models.count()).arg(getEntityName()));
        return result;
    }

    /**
     * @brief Get all models with pagination
     * @param page Page number (1-based)
     * @param pageSize Number of items per page
     * @param totalCount Output parameter for total count
     * @return List of models for the requested page
     */
    virtual QList<QSharedPointer<T>> getAllPaginated(int page, int pageSize, int &totalCount) {
        if (!ensureInitialized()) {
            totalCount = 0;
            return QList<QSharedPointer<T>>();
        }

        // Ensure valid pagination parameters
        if (page < 1) page = 1;
        if (pageSize < 1) pageSize = 10;

        // First get the total count
        QString countQuery = buildCountQuery();

        auto countResult = m_dbService->executeSingleSelectQuery(
            countQuery,
            QMap<QString, QVariant>(),
            [](const QSqlQuery& query) -> T* {
                T* countModel = new T();
                // Store the count in a property or tag that can be retrieved
                QJsonObject countData;
                countData["count"] = query.value(0).toInt();
                // This is a workaround to pass the count - implementation may vary
                countModel->setProperty("countData", countData);
                return countModel;
            }
        );

        if (countResult) {
            // Extract count from the model
            QJsonObject countData = (*countResult)->property("countData").toJsonObject();
            totalCount = countData["count"].toInt();
            delete *countResult;
        } else {
            totalCount = 0;
        }

        // Now get the actual data with limit and offset
        QString paginatedQuery = buildGetAllPaginatedQuery(page, pageSize);

        QMap<QString, QVariant> params;
        params["limit"] = pageSize;
        params["offset"] = (page - 1) * pageSize;

        auto models = m_dbService->executeSelectQuery(
            paginatedQuery,
            params,
            [this](const QSqlQuery& query) -> T* {
                return createModelFromQuery(query);
            }
        );

        QList<QSharedPointer<T>> result;
        for (auto model : models) {
            result.append(QSharedPointer<T>(model));
        }

        LOG_INFO(QString("Retrieved %1 %2 records (page %3 of %4)")
                .arg(models.count())
                .arg(getEntityName())
                .arg(page)
                .arg((totalCount + pageSize - 1) / pageSize));

        return result;
    }

    /**
     * @brief Remove a model by its ID
     * @param id The model ID
     * @return True if removal was successful
     */
    virtual bool remove(const QUuid& id) {
        if (!ensureInitialized()) {
            return false;
        }

        QMap<QString, QVariant> params;
        params[getIdParamName()] = id.toString(QUuid::WithoutBraces);

        QString query = buildRemoveQuery();

        bool success = m_dbService->executeModificationQuery(query, params);

        if (success) {
            LOG_INFO(QString("%1 removed successfully: %2").arg(getEntityName(), id.toString()));
        } else {
            LOG_ERROR(QString("Failed to remove %1: %2 - %3")
                     .arg(getEntityName(), id.toString(), m_dbService->lastError()));
        }

        return success;
    }

    /**
     * @brief Check if a record exists by ID
     * @param id The model ID
     * @return True if record exists
     */
    virtual bool exists(const QUuid& id) {
        if (!ensureInitialized()) {
            return false;
        }

        QMap<QString, QVariant> params;
        params[getIdParamName()] = id.toString(QUuid::WithoutBraces);

        QString query = buildExistsQuery();

        auto result = m_dbService->executeSingleSelectQuery(
            query,
            params,
            [](const QSqlQuery& query) -> T* {
                T* existsResult = new T();
                // Store the exists flag in a property or tag that can be retrieved
                QJsonObject existsData;
                existsData["exists"] = query.value(0).toBool();
                // This is a workaround to pass the exists flag - implementation may vary
                existsResult->setProperty("existsData", existsData);
                return existsResult;
            }
        );

        bool exists = false;
        if (result) {
            // Extract exists flag from the model
            QJsonObject existsData = (*result)->property("existsData").toJsonObject();
            exists = existsData["exists"].toBool();
            delete *result;
        }

        LOG_DEBUG(QString("%1 with ID %2 exists: %3")
                 .arg(getEntityName(), id.toString(), exists ? "yes" : "no"));

        return exists;
    }

    /**
     * @brief Begin a database transaction
     * @return True if transaction started successfully
     */
    bool beginTransaction() {
        if (!ensureInitialized()) {
            return false;
        }

        bool success = m_dbService->beginTransaction();
        if (success) {
            LOG_DEBUG(QString("Started transaction for %1 repository").arg(getEntityName()));
        } else {
            LOG_ERROR(QString("Failed to start transaction for %1 repository - %2")
                     .arg(getEntityName(), m_dbService->lastError()));
        }
        return success;
    }

    /**
     * @brief Commit a database transaction
     * @return True if transaction committed successfully
     */
    bool commitTransaction() {
        if (!ensureInitialized()) {
            return false;
        }

        bool success = m_dbService->commitTransaction();
        if (success) {
            LOG_DEBUG(QString("Committed transaction for %1 repository").arg(getEntityName()));
        } else {
            LOG_ERROR(QString("Failed to commit transaction for %1 repository - %2")
                     .arg(getEntityName(), m_dbService->lastError()));
        }
        return success;
    }

    /**
     * @brief Rollback a database transaction
     * @return True if transaction rolled back successfully
     */
    bool rollbackTransaction() {
        if (!ensureInitialized()) {
            return false;
        }

        bool success = m_dbService->rollbackTransaction();
        if (success) {
            LOG_DEBUG(QString("Rolled back transaction for %1 repository").arg(getEntityName()));
        } else {
            LOG_ERROR(QString("Failed to roll back transaction for %1 repository - %2")
                     .arg(getEntityName(), m_dbService->lastError()));
        }
        return success;
    }

    /**
     * @brief Execute a function within a transaction
     * @param operation The function to execute
     * @return True if operation was successful
     */
    bool executeInTransaction(std::function<bool()> operation) {
        if (!ensureInitialized()) {
            return false;
        }

        if (!beginTransaction()) {
            LOG_ERROR(QString("Failed to start transaction for %1").arg(getEntityName()));
            return false;
        }

        try {
            bool success = operation();

            if (success) {
                if (!commitTransaction()) {
                    LOG_ERROR(QString("Failed to commit transaction for %1").arg(getEntityName()));
                    rollbackTransaction();
                    return false;
                }
                return true;
            } else {
                LOG_WARNING(QString("Operation in transaction failed for %1, rolling back").arg(getEntityName()));
                rollbackTransaction();
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERROR(QString("Exception during transaction for %1: %2").arg(getEntityName(), e.what()));
            rollbackTransaction();
            return false;
        }
    }

    bool logQueryWithValues(const QString& queryTemplate, const QMap<QString, QVariant>& params)
    {
        // Create a copy of the query template that we'll replace parameters in
        QString queryWithValues = queryTemplate;

        // Replace each parameter with its value
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            QString paramName = ":" + it.key();
            QString paramValue;

            // Format the value based on its type
            if (it.value().type() == QVariant::String) {
                paramValue = "'" + it.value().toString() + "'";
            } else if (it.value().type() == QVariant::DateTime) {
                paramValue = "'" + it.value().toDateTime().toString(Qt::ISODate) + "'";
            } else if (it.value().isNull()) {
                paramValue = "NULL";
            } else {
                paramValue = it.value().toString();
            }

            // Replace all occurrences of the parameter in the query
            queryWithValues.replace(paramName, paramValue);
        }

        // Log the complete query with values
        LOG_INFO(QString("COMPLETE SQL QUERY: %1").arg(queryWithValues));

        return true;
    }

    /**
     * @brief Execute a custom select query that returns a single result
     * @param query The SQL query string
     * @param params The query parameters
     * @return Shared pointer to the model or nullptr if not found
     */
    QSharedPointer<T> executeSingleSelectQuery(const QString& query, const QMap<QString, QVariant>& params) {
        if (!ensureInitialized()) {
            return nullptr;
        }

        auto result = m_dbService->executeSingleSelectQuery(
            query,
            params,
            [this](const QSqlQuery& query) -> T* {
                return createModelFromQuery(query);
            }
        );

        if (result) {
            return QSharedPointer<T>(*result);
        }

        return nullptr;
    }

    /**
     * @brief Execute a custom select query that returns multiple results
     * @param query The SQL query string
     * @param params The query parameters
     * @return List of models
     */
    QList<QSharedPointer<T>> executeSelectQuery(const QString& query, const QMap<QString, QVariant>& params) {
        if (!ensureInitialized()) {
            return QList<QSharedPointer<T>>();
        }

        auto models = m_dbService->executeSelectQuery(
            query,
            params,
            [this](const QSqlQuery& query) -> T* {
                return createModelFromQuery(query);
            }
        );

        QList<QSharedPointer<T>> result;
        for (auto model : models) {
            result.append(QSharedPointer<T>(model));
        }

        LOG_DEBUG(QString("Custom query returned %1 %2 records").arg(models.count()).arg(getEntityName()));
        return result;
    }

    /**
     * @brief Execute a custom modification query (INSERT, UPDATE, DELETE)
     * @param query The SQL query string
     * @param params The query parameters
     * @return True if the operation was successful
     */
    bool executeModificationQuery(const QString& query, const QMap<QString, QVariant>& params) {
        if (!ensureInitialized()) {
            return false;
        }

        bool success = m_dbService->executeModificationQuery(query, params);

        if (success) {
            LOG_DEBUG("Custom modification query executed successfully");
        } else {
            LOG_ERROR(QString("Custom modification query failed: %1").arg(m_dbService->lastError()));
        }

        return success;
    }

    /**
     * @brief Get the database service
     * @return Pointer to the database service
     */
    DbService<T>* getDbService() const {
        return m_dbService;
    }

protected:

    /**
     * @brief Build the SQL query for saving with RETURNING clause
     * @return SQL query string
     */
    virtual QString buildSaveQueryWithReturning() {
        QString baseQuery = buildSaveQuery();

        // Check if the query already has a RETURNING clause
        if (!baseQuery.contains("RETURNING", Qt::CaseInsensitive)) {
            // Add RETURNING clause if not present
            return baseQuery + " RETURNING " + getIdParamName();
        }

        // Already has a RETURNING clause, return as is
        return baseQuery;
    }

    /**
     * @brief Create a model from a SQL query result
     * @param query The SQL query result
     * @return Pointer to the created model
     */
    virtual T* createModelFromQuery(const QSqlQuery& query) = 0;

    /**
     * @brief Get the entity name for logging
     * @return Entity name string
     */
    virtual QString getEntityName() const
    {
        return "BaseEntity";
    }

    /**
     * @brief Get the ID parameter name for queries
     * @return ID parameter name
     */
    virtual QString getIdParamName() const {
        return "id";
    }

    /**
     * @brief Get the ID from a model as a string
     * @param model The model
     * @return ID string
     */
    virtual QString getModelId(T* model) const = 0;

    /**
     * @brief Build the SQL query for checking if a record exists
     * @return SQL query string
     */
    virtual QString buildExistsQuery() {
        return QString("SELECT EXISTS(SELECT 1 FROM %1 WHERE %2 = :%2)")
               .arg(getTableName(), getIdParamName());
    }

    /**
     * @brief Build the SQL query for getting the total count
     * @return SQL query string
     */
    virtual QString buildCountQuery() {
        return QString("SELECT COUNT(*) FROM %1").arg(getTableName());
    }

    /**
     * @brief Build the SQL query for saving
     * @return SQL query string
     */
    virtual QString buildSaveQuery() = 0;

    /**
     * @brief Build the SQL query for updating
     * @return SQL query string
     */
    virtual QString buildUpdateQuery() = 0;

    /**
     * @brief Build the SQL query for getting by ID
     * @return SQL query string
     */
    virtual QString buildGetByIdQuery() = 0;

    /**
     * @brief Build the SQL query for getting all
     * @return SQL query string
     */
    virtual QString buildGetAllQuery() = 0;

    /**
     * @brief Build the SQL query for getting all with pagination
     * @param page Page number
     * @param pageSize Number of items per page
     * @return SQL query string
     */
    virtual QString buildGetAllPaginatedQuery(int page, int pageSize) {
        return QString("%1 LIMIT :limit OFFSET :offset").arg(buildGetAllQuery());
    }

    /**
     * @brief Build the SQL query for removing
     * @return SQL query string
     */
    virtual QString buildRemoveQuery() = 0;

    /**
     * @brief Get the database table name
     * @return Table name
     */
    virtual QString getTableName() const {
        // Default implementation, derived classes should override if different
        return getEntityName().toLower() + "s";
    }

    /**
     * @brief Prepare parameters for save query
     * @param model The model
     * @return Map of parameter names to values
     */
    virtual QMap<QString, QVariant> prepareParamsForSave(T* model) = 0;

    /**
     * @brief Prepare parameters for update query
     * @param model The model
     * @return Map of parameter names to values
     */
    virtual QMap<QString, QVariant> prepareParamsForUpdate(T* model) = 0;

    /**
     * @brief Validate a model before saving or updating
     * @param model The model to validate
     * @param errors List to store validation errors
     * @return True if model is valid
     */
    virtual bool validateModel(T* model, QStringList& errors) {
        // Base implementation always returns true
        // Derived classes should override this to perform actual validation
        return true;
    }

    /**
     * @brief Ensure the repository is initialized
     * @return True if initialized
     */
    bool ensureInitialized() {
        if (!m_initialized) {
            LOG_ERROR(QString("Cannot perform operation: %1 repository not initialized").arg(getEntityName()));
            return false;
        }
        return true;
    }

    /**
     * @brief Convert a QJsonObject to a string
     * @param json The JSON object
     * @return JSON string
     */
    QString jsonToString(const QJsonObject& json) {
        return QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
    }

    /**
     * @brief Convert a QJsonArray to a string
     * @param jsonArray The JSON array
     * @return JSON string
     */
    QString jsonArrayToString(const QJsonArray& jsonArray) {
        return QString::fromUtf8(QJsonDocument(jsonArray).toJson(QJsonDocument::Compact));
    }

    /**
     * @brief Parse a JSON string to a QJsonObject
     * @param jsonString The JSON string
     * @return JSON object
     */
    QJsonObject parseJsonString(const QString& jsonString) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        if (doc.isObject()) {
            return doc.object();
        }
        return QJsonObject();
    }

    /**
     * @brief Parse a JSON string to a QJsonArray
     * @param jsonString The JSON string
     * @return JSON array
     */
    QJsonArray parseJsonArrayString(const QString& jsonString) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        if (doc.isArray()) {
            return doc.array();
        }
        return QJsonArray();
    }

    DbService<T>* m_dbService;
    bool m_initialized;
};

#endif // BASEREPOSITORY_H

