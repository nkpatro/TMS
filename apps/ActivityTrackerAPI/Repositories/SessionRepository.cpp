#include "SessionRepository.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "Core/ModelFactory.h"

// Helper class for session chain statistics
class SessionChainStats : public SessionModel {
public:
    QString chainId;
    int totalSessions;
    QDateTime firstLogin;
    QDateTime lastActivity;
    double totalDurationSeconds;
    double totalGapSeconds;
    double realTimeSpanSeconds;
    double continuityPercentage;

    SessionChainStats() : SessionModel(),
        totalSessions(0),
        totalDurationSeconds(0),
        totalGapSeconds(0),
        realTimeSpanSeconds(0),
        continuityPercentage(0) {}
};

// Helper class for user session statistics
class UserStatsResult : public SessionModel {
public:
    int totalSessions;
    double totalSeconds;
    QDateTime firstLogin;
    QDateTime lastActivity;
    int uniqueMachines;

    UserStatsResult() : SessionModel(),
        totalSessions(0),
        totalSeconds(0),
        uniqueMachines(0) {}
};

// Helper class for AFK statistics
class AfkStatsResult : public SessionModel {
public:
    int totalAfk;
    double totalAfkSeconds;

    AfkStatsResult() : SessionModel(),
        totalAfk(0),
        totalAfkSeconds(0) {}
};

SessionRepository::SessionRepository(QObject *parent)
    : BaseRepository<SessionModel>(parent)
{
    LOG_DEBUG("SessionRepository created");
}

QString SessionRepository::getEntityName() const
{
    return "Session";
}

QString SessionRepository::getModelId(SessionModel* model) const
{
    return model->id().toString();
}

QString SessionRepository::buildSaveQuery()
{
    return "INSERT INTO sessions "
           "(user_id, login_time, logout_time, machine_id, ip_address, "
           "session_data, created_at, created_by, updated_at, updated_by, continued_from_session, "
           "continued_by_session, previous_session_end_time, time_since_previous_session) "
           "VALUES "
           "(:user_id, :login_time, :logout_time, :machine_id, "
           ":ip_address, :session_data, :created_at, :created_by, :updated_at, :updated_by, "
           ":continued_from_session, :continued_by_session, :previous_session_end_time, :time_since_previous_session)";
}

QString SessionRepository::buildUpdateQuery()
{
    return "UPDATE sessions SET "
           "user_id = :user_id, "
           "login_time = :login_time, "
           "logout_time = :logout_time, "
           "machine_id = :machine_id, "
           "ip_address = :ip_address, "
           "session_data = :session_data, "
           "updated_at = :updated_at, "
           "updated_by = :updated_by, "
           "continued_from_session = :continued_from_session, "
           "continued_by_session = :continued_by_session, "
           "previous_session_end_time = :previous_session_end_time, "
           "time_since_previous_session = :time_since_previous_session "
           "WHERE id = :id";
}

QString SessionRepository::buildGetByIdQuery()
{
    return "SELECT * FROM sessions WHERE id = :id";
}

QString SessionRepository::buildGetAllQuery()
{
    return "SELECT * FROM sessions ORDER BY login_time DESC";
}

QString SessionRepository::buildRemoveQuery()
{
    return "DELETE FROM sessions WHERE id = :id";
}

QMap<QString, QVariant> SessionRepository::prepareParamsForSave(SessionModel* session)
{
    QMap<QString, QVariant> params;

    // For standard non-NULL fields
    params["user_id"] = session->userId().toString(QUuid::WithoutBraces);
    params["machine_id"] = session->machineId().toString(QUuid::WithoutBraces);
    params["ip_address"] = session->ipAddress().toString();
    params["session_data"] = QString::fromUtf8(QJsonDocument(session->sessionData()).toJson());
    params["login_time"] = session->loginTime().toString(Qt::ISODate);
    params["created_at"] = session->createdAt().toString(Qt::ISODate);
    params["updated_at"] = session->updatedAt().toString(Qt::ISODate);
    params["time_since_previous_session"] = QString::number(session->timeSincePreviousSession());

    // For nullable UUID fields - use QVariant::Invalid for NULL
    params["created_by"] = session->createdBy().isNull() ? QVariant(QVariant::Invalid) : session->createdBy().toString(QUuid::WithoutBraces);
    params["updated_by"] = session->updatedBy().isNull() ? QVariant(QVariant::Invalid) : session->updatedBy().toString(QUuid::WithoutBraces);
    params["continued_from_session"] = session->continuedFromSession().isNull() ? QVariant(QVariant::Invalid) : session->continuedFromSession().toString(QUuid::WithoutBraces);
    params["continued_by_session"] = session->continuedBySession().isNull() ? QVariant(QVariant::Invalid) : session->continuedBySession().toString(QUuid::WithoutBraces);

    // For nullable timestamp fields
    params["logout_time"] = session->logoutTime().isValid() ? session->logoutTime().toString(Qt::ISODate) : QVariant(QVariant::Invalid);
    params["previous_session_end_time"] = session->previousSessionEndTime().isValid() ? session->previousSessionEndTime().toString(Qt::ISODate) : QVariant(QVariant::Invalid);

    return params;
}

QMap<QString, QVariant> SessionRepository::prepareParamsForUpdate(SessionModel* session)
{
    QMap<QString, QVariant> params = prepareParamsForSave(session);
    params["id"] = session->id().toString(QUuid::WithoutBraces);
    return params;
}

SessionModel* SessionRepository::createModelFromQuery(const QSqlQuery &query)
{
    // Use ModelFactory to create the model
    return ModelFactory::createSessionFromQuery(query);
}

QList<QSharedPointer<SessionModel>> SessionRepository::getByUserId(const QUuid &userId, bool activeOnly)
{
    LOG_DEBUG(QString("Retrieving sessions for user ID: %1, activeOnly: %2")
              .arg(userId.toString(), activeOnly ? "true" : "false"));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get sessions by user ID: Repository not initialized");
        return QList<QSharedPointer<SessionModel>>();
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);

    QString query;
    if (activeOnly) {
        query = "SELECT * FROM sessions WHERE user_id = :user_id AND logout_time IS NULL ORDER BY login_time DESC";
    } else {
        query = "SELECT * FROM sessions WHERE user_id = :user_id ORDER BY login_time DESC";
    }

    auto sessions = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> SessionModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<SessionModel>> result;
    for (auto session : sessions) {
        result.append(QSharedPointer<SessionModel>(session));
    }

    LOG_INFO(QString("Retrieved %1 sessions for user ID: %2")
             .arg(sessions.count())
             .arg(userId.toString()));
    return result;
}

QList<QSharedPointer<SessionModel>> SessionRepository::getByMachineId(const QUuid &machineId, bool activeOnly)
{
    LOG_DEBUG(QString("Retrieving sessions for machine ID: %1, activeOnly: %2")
              .arg(machineId.toString(), activeOnly ? "true" : "false"));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get sessions by machine ID: Repository not initialized");
        return QList<QSharedPointer<SessionModel>>();
    }

    QMap<QString, QVariant> params;
    params["machine_id"] = machineId.toString(QUuid::WithoutBraces);

    QString query;
    if (activeOnly) {
        query = "SELECT * FROM sessions WHERE machine_id = :machine_id AND logout_time IS NULL ORDER BY login_time DESC";
    } else {
        query = "SELECT * FROM sessions WHERE machine_id = :machine_id ORDER BY login_time DESC";
    }

    auto sessions = m_dbService->executeSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> SessionModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<SessionModel>> result;
    for (auto session : sessions) {
        result.append(QSharedPointer<SessionModel>(session));
    }

    LOG_INFO(QString("Retrieved %1 sessions for machine ID: %2")
             .arg(sessions.count())
             .arg(machineId.toString()));
    return result;
}

QSharedPointer<SessionModel> SessionRepository::getActiveSessionForUser(const QUuid &userId, const QUuid &machineId)
{
    LOG_DEBUG(QString("Getting active session for user ID: %1 and machine ID: %2")
              .arg(userId.toString(), machineId.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot get active session: Repository not initialized");
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["machine_id"] = machineId.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT * FROM sessions "
        "WHERE user_id = :user_id AND machine_id = :machine_id AND logout_time IS NULL "
        "ORDER BY login_time DESC "
        "LIMIT 1";

    // Log the query with values for debugging
    logQueryWithValues(query, params);

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [this](const QSqlQuery& query) -> SessionModel* {
            return createModelFromQuery(query);
        }
    );

    if (result) {
        // Debug log the session details to help with troubleshooting
        LOG_INFO(QString("Session record found: id=%1, userId=%2, machineId=%3, loginTime=%4, logoutTime=%5")
            .arg((*result)->id().toString())
            .arg((*result)->userId().toString())
            .arg((*result)->machineId().toString())
            .arg((*result)->loginTime().toString(Qt::ISODate))
            .arg((*result)->logoutTime().isValid() ? (*result)->logoutTime().toString(Qt::ISODate) : "NULL"));

        LOG_INFO(QString("Active session found for user ID: %1 and machine ID: %2")
                 .arg(userId.toString(), machineId.toString()));
        return QSharedPointer<SessionModel>(*result);
    } else {
        LOG_INFO(QString("No active session found for user ID: %1 and machine ID: %2")
                 .arg(userId.toString(), machineId.toString()));
        return nullptr;
    }
}

QList<QSharedPointer<SessionModel>> SessionRepository::getActiveSessions()
{
    LOG_DEBUG("Retrieving all active sessions");

    if (!isInitialized()) {
        LOG_ERROR("Cannot get active sessions: Repository not initialized");
        return QList<QSharedPointer<SessionModel>>();
    }

    QString query = "SELECT * FROM sessions WHERE logout_time IS NULL ORDER BY login_time DESC";
    logQueryWithValues(query, QMap<QString, QVariant>());

    auto sessions = m_dbService->executeSelectQuery(
        query,
        QMap<QString, QVariant>(),
        [this](const QSqlQuery& query) -> SessionModel* {
            return createModelFromQuery(query);
        }
    );

    QList<QSharedPointer<SessionModel>> result;
    for (auto session : sessions) {
        result.append(QSharedPointer<SessionModel>(session));
    }

    LOG_INFO(QString("Retrieved %1 active sessions").arg(sessions.count()));
    return result;
}

bool SessionRepository::createSessionWithTransaction(SessionModel *session)
{
    LOG_DEBUG(QString("createSessionWithTransaction"));

    if (!isInitialized()) {
        LOG_ERROR("Cannot create session: Repository not initialized");
        return false;
    }

    return executeInTransaction([this, session]() {
        // Save the session
        bool saveSuccess = save(session);
        if (!saveSuccess) {
            LOG_ERROR(QString("Failed to save session: %1").arg(session->id().toString()));
            return false;
        }

        // If continuing from a previous session, update the previous session
        if (!session->continuedFromSession().isNull()) {
            bool continueSuccess = continueSession(session->continuedFromSession(), session->id());
            if (!continueSuccess) {
                LOG_ERROR(QString("Failed to continue session: %1").arg(session->id().toString()));
                return false;
            }
        }

        LOG_INFO(QString("Session created successfully with transaction: %1").arg(session->id().toString()));
        return true;
    });
}

bool SessionRepository::endSession(const QUuid &id, const QDateTime &logoutTime)
{
    LOG_DEBUG(QString("Ending session with ID: %1").arg(id.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot end session: Repository not initialized");
        return false;
    }

    QMap<QString, QVariant> params;
    params["id"] = id.toString(QUuid::WithoutBraces);
    params["logout_time"] = logoutTime.toString(Qt::ISODate);
    params["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QString query =
        "UPDATE sessions SET "
        "logout_time = :logout_time, "
        "updated_at = :updated_at "
        "WHERE id = :id";

    bool success = m_dbService->executeModificationQuery(query, params);

    if (success) {
        LOG_INFO(QString("Session ended successfully: %1").arg(id.toString()));
    } else {
        LOG_ERROR(QString("Failed to end session: %1, error: %2")
                  .arg(id.toString(), m_dbService->lastError()));
    }

    return success;
}

bool SessionRepository::continueSession(const QUuid &previousSessionId, const QUuid &newSessionId)
{
    LOG_DEBUG(QString("Continuing session from %1 to %2")
              .arg(previousSessionId.toString(), newSessionId.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot continue session: Repository not initialized");
        return false;
    }

    // Begin transaction
    bool txStarted = m_dbService->beginTransaction();
    if (!txStarted) {
        LOG_ERROR("Failed to start transaction for session continuation");
        return false;
    }

    try {
        // Get the previous session
        QMap<QString, QVariant> prevParams;
        prevParams["id"] = previousSessionId.toString(QUuid::WithoutBraces);

        QString prevQuery = "SELECT * FROM sessions WHERE id = :id";

        auto prevResult = m_dbService->executeSingleSelectQuery(
            prevQuery,
            prevParams,
            [this](const QSqlQuery& query) -> SessionModel* {
                return createModelFromQuery(query);
            }
        );

        if (!prevResult) {
            LOG_ERROR(QString("Previous session not found: %1").arg(previousSessionId.toString()));
            m_dbService->rollbackTransaction();
            return false;
        }

        QSharedPointer<SessionModel> previousSession(*prevResult);

        // Get the new session
        QMap<QString, QVariant> newParams;
        newParams["id"] = newSessionId.toString(QUuid::WithoutBraces);

        QString newQuery = "SELECT * FROM sessions WHERE id = :id";

        auto newResult = m_dbService->executeSingleSelectQuery(
            newQuery,
            newParams,
            [this](const QSqlQuery& query) -> SessionModel* {
                return createModelFromQuery(query);
            }
        );

        if (!newResult) {
            LOG_ERROR(QString("New session not found: %1").arg(newSessionId.toString()));
            m_dbService->rollbackTransaction();
            return false;
        }

        QSharedPointer<SessionModel> newSession(*newResult);

        // Make sure the previous session has ended
        if (!previousSession->logoutTime().isValid()) {
            LOG_ERROR(QString("Previous session has not ended yet: %1").arg(previousSessionId.toString()));
            m_dbService->rollbackTransaction();
            return false;
        }

        // Update the previous session to point to the new session
        QMap<QString, QVariant> params1;
        params1["id"] = previousSessionId.toString(QUuid::WithoutBraces);
        params1["continued_by_session"] = newSessionId.toString(QUuid::WithoutBraces);
        params1["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        QString query1 =
            "UPDATE sessions SET "
            "continued_by_session = :continued_by_session, "
            "updated_at = :updated_at "
            "WHERE id = :id";

        LOG_DEBUG(QString("Updating previous session %1 to point to %2")
                  .arg(previousSessionId.toString(), newSessionId.toString()));

        bool success1 = m_dbService->executeModificationQuery(query1, params1);

        if (!success1) {
            LOG_ERROR(QString("Failed to update previous session: %1, error: %2")
                      .arg(previousSessionId.toString(), m_dbService->lastError()));
            m_dbService->rollbackTransaction();
            return false;
        }

        // Update the new session to point to the previous session
        QMap<QString, QVariant> params2;
        params2["id"] = newSessionId.toString(QUuid::WithoutBraces);
        params2["continued_from_session"] = previousSessionId.toString(QUuid::WithoutBraces);
        params2["previous_session_end_time"] = previousSession->logoutTime().toString(Qt::ISODate);
        params2["time_since_previous_session"] = QString::number(previousSession->logoutTime().secsTo(newSession->loginTime()));
        params2["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        QString query2 =
            "UPDATE sessions SET "
            "continued_from_session = :continued_from_session, "
            "previous_session_end_time = :previous_session_end_time, "
            "time_since_previous_session = :time_since_previous_session, "
            "updated_at = :updated_at "
            "WHERE id = :id";

        LOG_DEBUG(QString("Updating new session %1 to reference previous session %2")
                  .arg(newSessionId.toString(), previousSessionId.toString()));

        bool success2 = m_dbService->executeModificationQuery(query2, params2);

        if (!success2) {
            LOG_ERROR(QString("Failed to update new session: %1, error: %2")
                      .arg(newSessionId.toString(), m_dbService->lastError()));
            m_dbService->rollbackTransaction();
            return false;
        }

        // Commit transaction
        bool commitSuccess = m_dbService->commitTransaction();
        if (commitSuccess) {
            LOG_INFO(QString("Session continued successfully from %1 to %2")
                     .arg(previousSessionId.toString(), newSessionId.toString()));
        } else {
            LOG_ERROR(QString("Failed to commit transaction for session continuation from %1 to %2, error: %3")
                      .arg(previousSessionId.toString(), newSessionId.toString(), m_dbService->lastError()));
        }

        return commitSuccess;
    }
    catch (const std::exception& ex) {
        LOG_ERROR(QString("Exception during session continuation: %1").arg(ex.what()));
        m_dbService->rollbackTransaction();
        return false;
    }
}

QList<QSharedPointer<SessionModel>> SessionRepository::getSessionChain(const QUuid &id)
{
    QList<QSharedPointer<SessionModel>> sessionChain;

    LOG_DEBUG(QString("Getting session chain for session ID: %1").arg(id.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Repository not initialized");
        return sessionChain;
    }

    QMap<QString, QVariant> params;
    params["id"] = id.toString(QUuid::WithoutBraces);

    QString query =
        "SELECT * FROM get_session_chain(:id) "
        "ORDER BY position";

    LOG_DEBUG(QString("Executing session chain query with id: %1").arg(params["id"].toString()));

    try {
        auto sessions = m_dbService->executeSelectQuery(
            query,
            params,
            [this](const QSqlQuery& query) -> SessionModel* {
                // For stored procedure results that might not have all fields,
                // we handle this separately instead of using ModelFactory

                SessionModel* session = new SessionModel();

                // Populate the basic session data from the query
                if (!query.value("id").isNull()) {
                    session->setId(QUuid(query.value("id").toString()));
                }

                if (!query.value("user_id").isNull()) {
                    session->setUserId(QUuid(query.value("user_id").toString()));
                }

                if (!query.value("machine_id").isNull()) {
                    // In PostgreSQL, machine_id might be stored as text, not UUID
                    QString machineIdStr = query.value("machine_id").toString();
                    if (QUuid::fromString(machineIdStr).isNull()) {
                        // If it's not a valid UUID, use a null UUID or handle appropriately
                        LOG_WARNING(QString("Machine ID in session chain is not a valid UUID: %1").arg(machineIdStr));
                    } else {
                        session->setMachineId(QUuid(machineIdStr));
                    }
                }

                if (!query.value("login_time").isNull()) {
                    session->setLoginTime(query.value("login_time").toDateTime());
                }

                if (!query.value("logout_time").isNull()) {
                    session->setLogoutTime(query.value("logout_time").toDateTime());
                }

                return session;
            }
        );

        for (auto model : sessions) {
            if (model) {
                sessionChain.append(QSharedPointer<SessionModel>(model));
            }
        }

        LOG_INFO(QString("Retrieved %1 sessions in the chain for session ID: %2")
                .arg(sessionChain.count())
                .arg(id.toString()));
    }
    catch (const std::exception& ex) {
        LOG_ERROR(QString("Exception during getSessionChain: %1").arg(ex.what()));
    }

    return sessionChain;
}

QJsonObject SessionRepository::getSessionChainStats(const QUuid &id)
{
    LOG_DEBUG(QString("Getting session chain stats for session: %1").arg(id.toString()));

    QJsonObject stats;

    if (!isInitialized()) {
        LOG_ERROR("Cannot get session chain stats: Repository not initialized");
        return stats;
    }

    QMap<QString, QVariant> params;
    params["id"] = id.toString(QUuid::WithoutBraces);

    // PostgreSQL function that returns chain statistics
    QString query = "SELECT * FROM get_session_chain_stats(:id)";

    LOG_DEBUG(QString("Executing session chain stats query: %1").arg(query));
    LOG_DEBUG(QString("With parameters: id = %1").arg(params["id"].toString()));

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> SessionModel* {
            // Create a SessionChainStats object to store the results
            SessionChainStats* chainStats = new SessionChainStats();

            // Store the values in the chain stats object for easy access
            if (!query.value("chain_id").isNull()) {
                chainStats->chainId = query.value("chain_id").toString();
            }
            chainStats->totalSessions = query.value("total_sessions").toInt();
            chainStats->firstLogin = query.value("first_login").toDateTime();
            chainStats->lastActivity = query.value("last_activity").toDateTime();
            chainStats->totalDurationSeconds = query.value("total_duration_seconds").toDouble();
            chainStats->totalGapSeconds = query.value("total_gap_seconds").toDouble();
            chainStats->realTimeSpanSeconds = query.value("real_time_span_seconds").toDouble();
            chainStats->continuityPercentage = query.value("continuity_percentage").toDouble();

            // Also store values in a JSON object that will be added to the model's session data
            QJsonObject data;
            data["chain_id"] = chainStats->chainId;
            data["total_sessions"] = chainStats->totalSessions;
            data["first_login"] = chainStats->firstLogin.toString(Qt::ISODate);
            data["last_activity"] = chainStats->lastActivity.toString(Qt::ISODate);
            data["total_duration_seconds"] = chainStats->totalDurationSeconds;
            data["total_gap_seconds"] = chainStats->totalGapSeconds;
            data["real_time_span_seconds"] = chainStats->realTimeSpanSeconds;
            data["continuity_percentage"] = chainStats->continuityPercentage;

            // Store the data in the model's session data for easy retrieval
            chainStats->setSessionData(data);

            LOG_DEBUG(QString("Chain stats processed: %1 sessions, continuity: %2%")
                     .arg(chainStats->totalSessions)
                     .arg(chainStats->continuityPercentage, 0, 'f', 2));

            return chainStats;
        }
    );

    if (result) {
        // The query returned data, extract it from the result
        SessionChainStats* chainStats = static_cast<SessionChainStats*>(*result);

        // Get the data from the model's session data
        stats = chainStats->sessionData();

        LOG_INFO(QString("Retrieved chain stats for session %1: %2 sessions, %3% continuity")
                .arg(id.toString())
                .arg(chainStats->totalSessions)
                .arg(chainStats->continuityPercentage, 0, 'f', 2));

        delete chainStats; // Clean up
    } else {
        LOG_WARNING(QString("Failed to get session chain stats for session: %1").arg(id.toString()));
    }

    return stats;
}

QJsonObject SessionRepository::getUserSessionStats(const QUuid &userId, const QDateTime &startDate, const QDateTime &endDate)
{
    LOG_DEBUG(QString("Getting user session stats for user %1 from %2 to %3")
              .arg(userId.toString())
              .arg(startDate.toString(Qt::ISODate))
              .arg(endDate.toString(Qt::ISODate)));

    QJsonObject stats;

    if (!isInitialized()) {
        LOG_ERROR("Cannot get user session stats: Repository not initialized");
        return stats;
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["start_date"] = startDate.toString(Qt::ISODate);
    params["end_date"] = endDate.toString(Qt::ISODate);

    QString query =
        "SELECT "
        "COUNT(*) as total_sessions, "
        "COALESCE(SUM(EXTRACT(EPOCH FROM (COALESCE(logout_time, CURRENT_TIMESTAMP) - login_time))), 0) as total_seconds, "
        "COALESCE(MIN(login_time), CURRENT_TIMESTAMP) as first_login, "
        "COALESCE(MAX(COALESCE(logout_time, CURRENT_TIMESTAMP)), CURRENT_TIMESTAMP) as last_activity, "
        "COUNT(DISTINCT machine_id) as unique_machines "
        "FROM sessions "
        "WHERE user_id = :user_id "
        "AND login_time >= :start_date "
        "AND (logout_time IS NULL OR logout_time <= :end_date)";

    LOG_DEBUG(QString("Executing user session stats query: %1").arg(query));
    LOG_DEBUG(QString("With parameters: user_id = %1, start_date = %2, end_date = %3")
              .arg(params["user_id"].toString())
              .arg(params["start_date"].toString())
              .arg(params["end_date"].toString()));

    auto result = m_dbService->executeSingleSelectQuery(
        query,
        params,
        [](const QSqlQuery& query) -> SessionModel* {
            UserStatsResult* userStats = new UserStatsResult();

            userStats->totalSessions = query.value("total_sessions").toInt();
            userStats->totalSeconds = query.value("total_seconds").toDouble();
            userStats->firstLogin = query.value("first_login").toDateTime();
            userStats->lastActivity = query.value("last_activity").toDateTime();
            userStats->uniqueMachines = query.value("unique_machines").toInt();

            QJsonObject data;
            data["total_sessions"] = userStats->totalSessions;
            data["total_seconds"] = userStats->totalSeconds;
            data["first_login"] = userStats->firstLogin.toString(Qt::ISODate);
            data["last_activity"] = userStats->lastActivity.toString(Qt::ISODate);
            data["unique_machines"] = userStats->uniqueMachines;

            userStats->setSessionData(data);

            LOG_DEBUG(QString("User stats processed: %1 sessions, %2 seconds total")
                     .arg(userStats->totalSessions)
                     .arg(userStats->totalSeconds));

            return userStats;
        }
    );

    if (result) {
        LOG_DEBUG("User session stats query executed successfully");

        UserStatsResult* userStats = static_cast<UserStatsResult*>(*result);

        // Extract data from the result
        stats = userStats->sessionData();

        // Calculate daily averages
        QDateTime first = userStats->firstLogin;
        QDateTime last = userStats->lastActivity;
        int days = first.daysTo(last) + 1;

        if (days > 0) {
            stats["average_seconds_per_day"] = userStats->totalSeconds / days;
            stats["average_sessions_per_day"] = (double)userStats->totalSessions / days;

            LOG_DEBUG(QString("Days: %1, Avg seconds per day: %2")
                     .arg(days)
                     .arg(stats["average_seconds_per_day"].toDouble()));
        }

        delete userStats;
    } else {
        LOG_ERROR("Failed to get user session stats");
    }

    // Get AFK stats
    QMap<QString, QVariant> afkParams;
    afkParams["user_id"] = userId.toString(QUuid::WithoutBraces);
    afkParams["start_date"] = startDate.toString(Qt::ISODate);
    afkParams["end_date"] = endDate.toString(Qt::ISODate);

    QString afkQuery =
        "SELECT "
        "COUNT(*) as total_afk, "
        "COALESCE(SUM(EXTRACT(EPOCH FROM (COALESCE(end_time, CURRENT_TIMESTAMP) - start_time))), 0) as total_afk_seconds "
        "FROM afk_periods ap "
        "JOIN sessions s ON ap.session_id = s.id "
        "WHERE s.user_id = :user_id "
        "AND ap.start_time >= :start_date "
        "AND (ap.end_time IS NULL OR ap.end_time <= :end_date)";

    LOG_DEBUG(QString("Executing AFK stats query: %1").arg(afkQuery));
    LOG_DEBUG(QString("With parameters: user_id = %1, start_date = %2, end_date = %3")
              .arg(afkParams["user_id"].toString())
              .arg(afkParams["start_date"].toString())
              .arg(afkParams["end_date"].toString()));

    auto afkResult = m_dbService->executeSingleSelectQuery(
        afkQuery,
        afkParams,
        [](const QSqlQuery& query) -> SessionModel* {
            AfkStatsResult* afkStats = new AfkStatsResult();

            afkStats->totalAfk = query.value("total_afk").toInt();
            afkStats->totalAfkSeconds = query.value("total_afk_seconds").toDouble();

            LOG_DEBUG(QString("AFK stats processed: %1 periods, %2 seconds total")
                     .arg(afkStats->totalAfk)
                     .arg(afkStats->totalAfkSeconds));

            return afkStats;
        }
    );

    if (afkResult) {
        LOG_DEBUG("AFK stats query executed successfully");

        AfkStatsResult* afkStats = static_cast<AfkStatsResult*>(*afkResult);

        stats["total_afk_periods"] = afkStats->totalAfk;
        stats["total_afk_seconds"] = afkStats->totalAfkSeconds;

        if (stats["total_seconds"].toDouble() > 0) {
            double afkPercent = (afkStats->totalAfkSeconds / stats["total_seconds"].toDouble()) * 100.0;
            stats["afk_percentage"] = afkPercent;
            stats["active_seconds"] = stats["total_seconds"].toDouble() - afkStats->totalAfkSeconds;

            LOG_DEBUG(QString("AFK percentage: %1%, Active seconds: %2")
                     .arg(afkPercent, 0, 'f', 2)
                     .arg(stats["active_seconds"].toDouble()));
        }

        delete afkStats;
    } else {
        LOG_ERROR("Failed to get AFK stats");
    }

    LOG_INFO(QString("Completed getting user session stats for user %1").arg(userId.toString()));
    LOG_DEBUG(QString("Stats object contains %1 fields").arg(stats.size()));

    return stats;
}

// Add these implementations to SessionRepository.cpp
QSharedPointer<SessionModel> SessionRepository::getSessionForDay(const QUuid& userId, const QUuid& machineId, const QDate& date)
{
    LOG_DEBUG(QString("Getting session for user ID: %1, machine ID: %2, date: %3")
        .arg(userId.toString())
        .arg(machineId.toString())
        .arg(date.toString(Qt::ISODate)));

    if (!ensureInitialized()) {
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["machine_id"] = machineId.toString(QUuid::WithoutBraces);
    params["start_of_day"] = QDateTime(date, QTime(0, 0, 0), Qt::UTC).toString(Qt::ISODate);
    params["end_of_day"] = QDateTime(date, QTime(23, 59, 59, 999), Qt::UTC).toString(Qt::ISODate);

    QString query =
        "SELECT * FROM sessions WHERE user_id = :user_id AND machine_id = :machine_id "
        "AND login_time >= :start_of_day AND login_time <= :end_of_day "
        "ORDER BY login_time DESC LIMIT 1";

    auto result = executeSingleSelectQuery(query, params);

    if (result) {
        LOG_INFO(QString("Found session for user ID: %1, machine ID: %2, date: %3 - Session ID: %4")
            .arg(userId.toString())
            .arg(machineId.toString())
            .arg(date.toString(Qt::ISODate))
            .arg(result->id().toString()));
    }
    else {
        LOG_INFO(QString("No session found for user ID: %1, machine ID: %2, date: %3")
            .arg(userId.toString())
            .arg(machineId.toString())
            .arg(date.toString(Qt::ISODate)));
    }

    return result;
}

QSharedPointer<SessionModel> SessionRepository::createOrReuseSessionWithTransaction(
    const QUuid& userId,
    const QUuid& machineId,
    const QDateTime& currentDateTime,
    const QHostAddress& ipAddress,
    const QJsonObject& sessionData,
    bool isRemote,
    const QString& terminalSessionId)
{
    LOG_DEBUG(QString("Creating or reusing session for user %1 on machine %2")
        .arg(userId.toString(), machineId.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot create session: Repository not initialized");
        return nullptr;
    }

    QSharedPointer<SessionModel> resultSession;

    bool success = executeInTransaction([&]() {
        // Check for active sessions by directly getting them from repository methods
        QList<QSharedPointer<SessionModel>> activeSessions = getActiveSessions();

        // Filter to find sessions for the specific user and machine
        QUuid lastActiveSessionId;

        for (const auto& session : activeSessions) {
            if (session->userId() == userId && session->machineId() == machineId) {
                lastActiveSessionId = session->id();

                // End the session
                session->setLogoutTime(currentDateTime);
                session->setUpdatedAt(currentDateTime);
                session->setUpdatedBy(userId);

                bool updateSuccess = update(session.data());
                if (!updateSuccess) {
                    LOG_ERROR(QString("Failed to end existing session: %1").arg(session->id().toString()));
                    return false;
                }

                LOG_INFO(QString("Ended active session: %1 for user %2 on machine %3")
                    .arg(session->id().toString(), userId.toString(), machineId.toString()));
            }
        }

        // 2. Check for a session created today that might be closed
        QDate currentDate = currentDateTime.date();
        auto todaySession = getSessionForDay(userId, machineId, currentDate);

        // 3. Create a new session or reopen today's session
        if (todaySession && todaySession->logoutTime().isValid()) {
            // Today's session exists but is closed - reopen it
            LOG_INFO(QString("Reopening closed session for today: %1").arg(todaySession->id().toString()));

            // Clear the logout time to reactivate the session
            todaySession->setLogoutTime(QDateTime()); // Set to invalid/null
            todaySession->setUpdatedAt(currentDateTime);
            todaySession->setUpdatedBy(userId);

            bool updateSuccess = update(todaySession.data());
            if (!updateSuccess) {
                LOG_ERROR(QString("Failed to reopen session: %1").arg(todaySession->id().toString()));
                return false;
            }

            resultSession = todaySession;
        }
        else if (todaySession && !todaySession->logoutTime().isValid()) {
            // Today's session exists and is active - just use it
            LOG_INFO(QString("Using existing active session for today: %1").arg(todaySession->id().toString()));
            resultSession = todaySession;
        }
        else {
            // No session for today - create a new one
            LOG_INFO("Creating new session");
            SessionModel* newSession = new SessionModel();
            newSession->setId(QUuid::createUuid());
            newSession->setUserId(userId);
            newSession->setLoginTime(currentDateTime);
            newSession->setMachineId(machineId);
            newSession->setIpAddress(ipAddress);
            newSession->setSessionData(sessionData);

            // Set session continuity information if there was a previous session
            if (!lastActiveSessionId.isNull()) {
                auto lastSession = getById(lastActiveSessionId);
                if (lastSession) {
                    newSession->setContinuedFromSession(lastActiveSessionId);
                    newSession->setPreviousSessionEndTime(lastSession->logoutTime());
                    newSession->setTimeSincePreviousSession(
                        lastSession->logoutTime().secsTo(currentDateTime));

                    // Update the previous session to link to this one
                    lastSession->setContinuedBySession(newSession->id());
                    if (!update(lastSession.data())) {
                        LOG_ERROR(QString("Failed to update continuity link in previous session: %1")
                            .arg(lastSession->id().toString()));
                        delete newSession;
                        return false;
                    }
                }
            }

            // Set metadata
            newSession->setCreatedBy(userId);
            newSession->setUpdatedBy(userId);
            newSession->setCreatedAt(currentDateTime);
            newSession->setUpdatedAt(currentDateTime);

            // Save the session
            if (!save(newSession)) {
                LOG_ERROR("Failed to save new session");
                delete newSession;
                return false;
            }

            resultSession = QSharedPointer<SessionModel>(newSession);
        }

        return true;
    });

    if (!success) {
        LOG_ERROR("Transaction failed during session creation/reuse");
        return nullptr;
    }

    return resultSession;
}

