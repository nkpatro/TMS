#include "DisciplineRepository.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "Core/ModelFactory.h"

// Helper class for discipline statistics
class DisciplineStatsResult : public DisciplineModel {
public:
    int totalReferences;
    int activeUsers;
    QDateTime lastUsed;

    DisciplineStatsResult() : DisciplineModel(),
        totalReferences(0),
        activeUsers(0) {}
};

DisciplineRepository::DisciplineRepository(QObject *parent)
    : BaseRepository<DisciplineModel>(parent)
{
    LOG_DEBUG("DisciplineRepository created");
}

QString DisciplineRepository::getEntityName() const
{
    return "Discipline";
}

QString DisciplineRepository::getModelId(DisciplineModel* model) const
{
    return model->id().toString();
}

QString DisciplineRepository::buildSaveQuery()
{
    return "INSERT INTO disciplines "
           "(code, name, description, created_at, created_by, updated_at, updated_by) "
           "VALUES "
           "(:code, :name, :description, :created_at, :created_by, :updated_at, :updated_by) "
           "RETURNING id";
}

QString DisciplineRepository::buildUpdateQuery()
{
    return "UPDATE disciplines SET "
           "code = :code, "
           "name = :name, "
           "description = :description, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by "
           "WHERE id = :id";
}

QString DisciplineRepository::buildGetByIdQuery()
{
    return "SELECT * FROM disciplines WHERE id = :id";
}

QString DisciplineRepository::buildGetAllQuery()
{
    return "SELECT * FROM disciplines ORDER BY name";
}

QString DisciplineRepository::buildRemoveQuery()
{
    return "DELETE FROM disciplines WHERE id = :id";
}

QMap<QString, QVariant> DisciplineRepository::prepareParamsForSave(DisciplineModel* discipline)
{
    QMap<QString, QVariant> params;
    params["code"] = discipline->code();
    params["name"] = discipline->name();
    params["description"] = discipline->description();
    params["created_at"] = discipline->createdAt().toUTC();
    params["created_by"] = discipline->createdBy().isNull() ? QVariant(QVariant::Invalid) : discipline->createdBy().toString(QUuid::WithoutBraces);
    params["updated_at"] = discipline->updatedAt().toUTC();
    params["updated_by"] = discipline->updatedBy().isNull() ? QVariant(QVariant::Invalid) : discipline->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

QMap<QString, QVariant> DisciplineRepository::prepareParamsForUpdate(DisciplineModel* discipline)
{
    QMap<QString, QVariant> params;
    params["id"] = discipline->id().toString(QUuid::WithoutBraces);
    params["code"] = discipline->code();
    params["name"] = discipline->name();
    params["description"] = discipline->description();
    params["updated_at"] = QDateTime::currentDateTimeUtc().toUTC();
    params["updated_by"] = discipline->updatedBy().isNull() ? QVariant(QVariant::Invalid) : discipline->updatedBy().toString(QUuid::WithoutBraces);

    return params;
}

DisciplineModel* DisciplineRepository::createModelFromQuery(const QSqlQuery &query)
{
    return ModelFactory::createDisciplineFromQuery(query);
}

bool DisciplineRepository::remove(const QUuid &id)
{
    LOG_DEBUG(QString("Removing discipline: %1").arg(id.toString()));

    if (!ensureInitialized()) {
        return false;
    }

    // Use executeInTransaction to handle the transaction
    return executeInTransaction([this, id]() {
        // First check if this discipline is referenced in sessions
        QMap<QString, QVariant> checkParams;
        checkParams["discipline_id"] = id.toString(QUuid::WithoutBraces);

        QString checkQuery = "SELECT COUNT(*) as ref_count FROM sessions WHERE discipline_id = :discipline_id";

        LOG_DEBUG(QString("Checking for discipline references: %1").arg(checkQuery));
        LOG_DEBUG(QString("With parameters: discipline_id=%1").arg(checkParams["discipline_id"].toString()));

        auto checkResult = m_dbService->executeSingleSelectQuery(
            checkQuery,
            checkParams,
            [](const QSqlQuery& query) -> DisciplineModel* {
                // We're just using this to get the count
                DisciplineModel* model = ModelFactory::createDefaultDiscipline();
                QJsonObject data;
                data["ref_count"] = query.value("ref_count").toInt();
                model->setDescription(QString::number(data["ref_count"].toInt()));
                return model;
            }
        );

        if (checkResult) {
            int refCount = (*checkResult)->description().toInt();
            delete *checkResult;

            if (refCount > 0) {
                LOG_WARNING(QString("Cannot remove discipline %1: Referenced in %2 sessions").arg(id.toString()).arg(refCount));
                return false;
            }
        }

        // Proceed with deletion
        QMap<QString, QVariant> params;
        params["id"] = id.toString(QUuid::WithoutBraces);

        QString query = buildRemoveQuery();

        LOG_DEBUG(QString("Executing remove query: %1").arg(query));
        LOG_DEBUG(QString("With parameters: id=%1").arg(params["id"].toString()));

        bool success = m_dbService->executeModificationQuery(query, params);

        if (success) {
            LOG_INFO(QString("Discipline removed successfully: %1").arg(id.toString()));
            return true;
        } else {
            LOG_ERROR(QString("Failed to remove discipline: %1 - %2")
                    .arg(id.toString(), m_dbService->lastError()));
            return false;
        }
    });
}

QSharedPointer<DisciplineModel> DisciplineRepository::getByCode(const QString &code)
{
    LOG_DEBUG(QString("Getting discipline by code: %1").arg(code));

    if (!ensureInitialized()) {
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["code"] = code;

    QString query = "SELECT * FROM disciplines WHERE code = :code";

    LOG_DEBUG(QString("Executing getByCode query: %1").arg(query));
    LOG_DEBUG(QString("With parameters: code=%1").arg(params["code"].toString()));

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> DisciplineModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        LOG_INFO(QString("Discipline found with code: %1 (%2)").arg(code, (*result)->id().toString()));
        return QSharedPointer<DisciplineModel>(*result);
    }

    LOG_WARNING(QString("Discipline not found with code: %1").arg(code));
    return nullptr;
}

QList<QSharedPointer<DisciplineModel>> DisciplineRepository::getByName(const QString &name)
{
    LOG_DEBUG(QString("Retrieving disciplines by name: %1").arg(name));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<DisciplineModel>>();
    }

    QMap<QString, QVariant> params;
    params["name"] = "%" + name + "%";

    QString query = "SELECT * FROM disciplines WHERE name ILIKE :name ORDER BY name";

    LOG_DEBUG(QString("Executing getByName query: %1").arg(query));
    LOG_DEBUG(QString("With parameters: name=%1").arg(params["name"].toString()));

    auto disciplines = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> DisciplineModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<DisciplineModel>> result;
    for (auto discipline : disciplines) {
        result.append(QSharedPointer<DisciplineModel>(discipline));
    }

    LOG_INFO(QString("Retrieved %1 disciplines by name '%2'").arg(disciplines.count()).arg(name));
    return result;
}

QList<QSharedPointer<DisciplineModel>> DisciplineRepository::search(const QString &term)
{
    LOG_DEBUG(QString("Searching disciplines with term: %1").arg(term));

    if (!ensureInitialized()) {
        return QList<QSharedPointer<DisciplineModel>>();
    }

    QMap<QString, QVariant> params;
    params["term"] = "%" + term + "%";

    QString query =
        "SELECT * FROM disciplines WHERE "
        "name ILIKE :term OR "
        "code ILIKE :term OR "
        "description ILIKE :term "
        "ORDER BY name";

    LOG_DEBUG(QString("Executing search query: %1").arg(query));
    LOG_DEBUG(QString("With parameters: term=%1").arg(params["term"].toString()));

    auto disciplines = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> DisciplineModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<DisciplineModel>> result;
    for (auto discipline : disciplines) {
        result.append(QSharedPointer<DisciplineModel>(discipline));
    }

    LOG_INFO(QString("Search returned %1 disciplines for term '%2'").arg(disciplines.count()).arg(term));
    return result;
}

QJsonObject DisciplineRepository::getDisciplineStats(const QUuid &id)
{
    LOG_DEBUG(QString("Getting statistics for discipline: %1").arg(id.toString()));

    QJsonObject stats;

    if (!ensureInitialized()) {
        return stats;
    }

    QMap<QString, QVariant> params;
    params["discipline_id"] = id.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT "
        "COUNT(*) as total_references, "
        "COUNT(DISTINCT user_id) as active_users, "
        "MAX(login_time) as last_used "
        "FROM sessions "
        "WHERE discipline_id = :discipline_id";

    LOG_DEBUG(QString("Executing discipline stats query: %1").arg(query));
    LOG_DEBUG(QString("With parameters: discipline_id=%1").arg(params["discipline_id"].toString()));

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> DisciplineModel* {
            DisciplineStatsResult* statsResult = new DisciplineStatsResult();

            statsResult->totalReferences = query.value("total_references").toInt();
            statsResult->activeUsers = query.value("active_users").toInt();

            if (!query.value("last_used").isNull()) {
                statsResult->lastUsed = query.value("last_used").toDateTime();
            }

            QJsonObject data;
            data["total_references"] = statsResult->totalReferences;
            data["active_users"] = statsResult->activeUsers;

            if (statsResult->lastUsed.isValid()) {
                data["last_used"] = statsResult->lastUsed.toUTC().toString();
            } else {
                data["last_used"] = QJsonValue::Null;
            }

            // Store in model for easy extraction
            QJsonDocument doc(data);
            statsResult->setDescription(QString::fromUtf8(doc.toJson()));

            return statsResult;
        }
    );

    if (result) {
        DisciplineStatsResult* statsResult = static_cast<DisciplineStatsResult*>(*result);

        // Extract JSON from description where we stored it
        QJsonDocument doc = QJsonDocument::fromJson(statsResult->description().toUtf8());
        stats = doc.object();

        LOG_INFO(QString("Retrieved stats for discipline %1: %2 references, %3 active users")
                .arg(id.toString())
                .arg(statsResult->totalReferences)
                .arg(statsResult->activeUsers));

        delete statsResult;
    } else {
        LOG_WARNING(QString("Failed to get stats for discipline: %1").arg(id.toString()));
    }

    return stats;
}

bool DisciplineRepository::batchCreate(const QList<DisciplineModel*> &disciplines)
{
    LOG_DEBUG(QString("Batch creating %1 disciplines").arg(disciplines.size()));

    if (!ensureInitialized()) {
        return false;
    }

    if (disciplines.isEmpty()) {
        LOG_WARNING("No disciplines provided for batch creation");
        return true; // Nothing to do, so technically successful
    }

    // Use the executeInTransaction method from BaseRepository
    return executeInTransaction([this, &disciplines]() {
        int successCount = 0;

        for (DisciplineModel* discipline : disciplines) {
            if (save(discipline)) {
                successCount++;
            } else {
                LOG_ERROR(QString("Failed to save discipline in batch: %1 (%2)")
                        .arg(discipline->name(), discipline->id().toString()));
            }
        }

        // Return true if at least one was created successfully
        return successCount > 0;
    });
}

QSharedPointer<DisciplineModel> DisciplineRepository::getOrCreate(const QString &code, const QString &name, const QString &description)
{
    LOG_DEBUG(QString("Getting or creating discipline with code: %1").arg(code));

    if (!ensureInitialized()) {
        return nullptr;
    }

    // First try to get by code
    auto existingDiscipline = getByCode(code);
    if (existingDiscipline) {
        LOG_INFO(QString("Found existing discipline with code %1: %2")
                .arg(code, existingDiscipline->name()));
        return existingDiscipline;
    }

    // Need to create a new one
    LOG_DEBUG(QString("No discipline found with code %1, creating new one").arg(code));

    // Use ModelFactory to create the discipline
    DisciplineModel* newDiscipline = ModelFactory::createDefaultDiscipline(name);
    newDiscipline->setCode(code);
    newDiscipline->setDescription(description);

    if (save(newDiscipline)) {
        LOG_INFO(QString("Created new discipline with code %1: %2 (%3)")
                .arg(code, name, newDiscipline->id().toString()));
        return QSharedPointer<DisciplineModel>(newDiscipline);
    } else {
        LOG_ERROR(QString("Failed to create new discipline with code %1").arg(code));
        delete newDiscipline;
        return nullptr;
    }
}

