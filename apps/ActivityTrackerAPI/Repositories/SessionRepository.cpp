#include "SessionRepository.h"
#include "SessionEventRepository.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "EventTypes.h"
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
           ":continued_from_session, :continued_by_session, :previous_session_end_time, :time_since_previous_session) "
           "RETURNING id";
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

    if (userId.isNull())
    {
        LOG_DEBUG(QString("Getting active session for user ID: %1 and machine ID: %2")
              .arg(userId.toString(), machineId.toString()));
    }

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
        // Get the specific active session for this user and machine
        auto activeSession = getActiveSessionForUser(userId, machineId);
        QUuid lastActiveSessionId;

        if (activeSession) {
            lastActiveSessionId = activeSession->id();

            LOG_INFO(QString("Found active session: %1 with login time: %2")
                .arg(activeSession->id().toString())
                .arg(activeSession->loginTime().toString(Qt::ISODate)));

            // Use safe method to end the session
            SessionEventRepository* eventRepo = nullptr;
            if (m_sessionEventRepository) {
                eventRepo = m_sessionEventRepository;
            }

            // Get the time of the last activity for this session to use as logout time
            QDateTime logoutTime = currentDateTime;
            if (eventRepo && eventRepo->isInitialized()) {
                // We could find the latest activity, but for simplicity using current time
                // Future enhancement: query for the latest activity event time
            }

            bool updateSuccess = safeEndSession(activeSession->id(), logoutTime, eventRepo);

            if (!updateSuccess) {
                LOG_ERROR(QString("Failed to end existing session: %1").arg(activeSession->id().toString()));
                return false;
            }

            LOG_INFO(QString("Ended active session: %1 for user %2 on machine %3")
                .arg(activeSession->id().toString(), userId.toString(), machineId.toString()));
        }

        // Check for a session created today that might be closed
        QDate currentDate = currentDateTime.date();

        // Direct SQL query to find today's session
        QMap<QString, QVariant> params;
        params["user_id"] = userId.toString(QUuid::WithoutBraces);
        params["machine_id"] = machineId.toString(QUuid::WithoutBraces);
        params["start_of_day"] = QDateTime(currentDate, QTime(0, 0, 0), Qt::UTC).toString(Qt::ISODate);
        params["end_of_day"] = QDateTime(currentDate, QTime(23, 59, 59, 999), Qt::UTC).toString(Qt::ISODate);

        QString query =
            "SELECT * FROM sessions WHERE user_id = :user_id AND machine_id = :machine_id "
            "AND login_time >= :start_of_day AND login_time <= :end_of_day "
            "ORDER BY login_time DESC LIMIT 1";

        auto todaySession = executeSingleSelectQuery(query, params);

        if (todaySession && todaySession->logoutTime().isValid()) {
            // Today's session exists but is closed - reopen it
            LOG_INFO(QString("Reopening closed session for today: %1").arg(todaySession->id().toString()));

            // Use safe method to reopen session
            SessionEventRepository* eventRepo = nullptr;
            if (m_sessionEventRepository) {
                eventRepo = m_sessionEventRepository;
            }

            bool reopenSuccess = safeReopenSession(todaySession->id(), currentDateTime, eventRepo);

            if (!reopenSuccess) {
                LOG_ERROR(QString("Failed to reopen session: %1").arg(todaySession->id().toString()));
                return false;
            }

            // Refresh the session data
            todaySession = getById(todaySession->id());
            resultSession = todaySession;

            // Verify login event exists - note we don't update the original login_time
            if (eventRepo && eventRepo->isInitialized()) {
                // First ensure there is a login event for the original login time
                if (!hasLoginEvent(todaySession->id(), todaySession->loginTime(), eventRepo)) {
                    LOG_WARNING(QString("Reopened session %1 is missing original login event. Creating one.")
                              .arg(todaySession->id().toString()));

                    bool eventSuccess = createSessionLoginEvent(
                        todaySession->id(),
                        userId,
                        machineId,
                        todaySession->loginTime(), // Use original session login time
                        eventRepo,
                        isRemote,
                        terminalSessionId
                    );

                    if (!eventSuccess) {
                        LOG_ERROR(QString("Failed to create login event for reopened session %1")
                            .arg(todaySession->id().toString()));
                        // Continue anyway, as we have a backup below
                    }
                }

                // Create a login event for the current reopen time
                if (!hasLoginEvent(todaySession->id(), currentDateTime, eventRepo)) {
                    LOG_INFO(QString("Creating new login event for reopened session %1")
                           .arg(todaySession->id().toString()));

                    // Create a login event with current time (reopening time)
                    SessionEventModel* loginEvent = new SessionEventModel();
                    loginEvent->setId(QUuid::createUuid());
                    loginEvent->setSessionId(todaySession->id());
                    loginEvent->setEventType(EventTypes::SessionEventType::Login);
                    loginEvent->setEventTime(currentDateTime);
                    loginEvent->setUserId(userId);
                    loginEvent->setMachineId(machineId);
                    loginEvent->setIsRemote(isRemote);

                    if (!terminalSessionId.isEmpty()) {
                        loginEvent->setTerminalSessionId(terminalSessionId);
                    }

                    // Add metadata explaining this is a reopened session
                    QJsonObject eventData;
                    eventData["reason"] = "session_reopened";
                    eventData["auto_generated"] = true;
                    eventData["original_login_time"] = todaySession->loginTime().toString(Qt::ISODate);
                    loginEvent->setEventData(eventData);

                    // Set metadata
                    loginEvent->setCreatedAt(QDateTime::currentDateTimeUtc());
                    loginEvent->setUpdatedAt(QDateTime::currentDateTimeUtc());
                    loginEvent->setCreatedBy(userId);
                    loginEvent->setUpdatedBy(userId);

                    bool eventSuccess = eventRepo->save(loginEvent);
                    delete loginEvent;

                    if (!eventSuccess) {
                        LOG_WARNING(QString("Failed to create login event for reopened session %1")
                                  .arg(todaySession->id().toString()));
                        // Continue anyway since session creation is more important
                    }
                }
            }
        }
        else if (todaySession && !todaySession->logoutTime().isValid()) {
            // Today's session exists and is active - just use it
            LOG_INFO(QString("Using existing active session for today: %1").arg(todaySession->id().toString()));

            // Check if this session has a login event, if not create one
            SessionEventRepository* eventRepo = nullptr;
            if (m_sessionEventRepository) {
                eventRepo = m_sessionEventRepository;
            }

            if (eventRepo && eventRepo->isInitialized()) {
                // Check for and create a login event that uses the original session login time
                if (!hasLoginEvent(todaySession->id(), todaySession->loginTime(), eventRepo)) {
                    LOG_WARNING(QString("Active session %1 is missing login event. Creating one for original login time.")
                              .arg(todaySession->id().toString()));

                    bool eventSuccess = createSessionLoginEvent(
                        todaySession->id(),
                        userId,
                        machineId,
                        todaySession->loginTime(), // Use original session login time
                        eventRepo,
                        isRemote,
                        terminalSessionId
                    );

                    if (!eventSuccess) {
                        LOG_ERROR(QString("Failed to create login event for existing session %1")
                            .arg(todaySession->id().toString()));
                        // Continue anyway, as we can try again below
                    }
                }

                // Create a login event for this current login time if it doesn't exist
                // Only create if the current time is significantly different from the original login
                if (qAbs(currentDateTime.secsTo(todaySession->loginTime())) > 300 && // More than 5 minutes different
                    !hasLoginEvent(todaySession->id(), currentDateTime, eventRepo)) {

                    LOG_INFO(QString("Creating additional login event at current time for session %1")
                           .arg(todaySession->id().toString()));

                    // Create a new login event for current time, but don't update the session's login_time
                    SessionEventModel* loginEvent = new SessionEventModel();
                    loginEvent->setId(QUuid::createUuid());
                    loginEvent->setSessionId(todaySession->id());
                    loginEvent->setEventType(EventTypes::SessionEventType::Login);
                    loginEvent->setEventTime(currentDateTime);
                    loginEvent->setUserId(userId);
                    loginEvent->setMachineId(machineId);
                    loginEvent->setIsRemote(isRemote);

                    if (!terminalSessionId.isEmpty()) {
                        loginEvent->setTerminalSessionId(terminalSessionId);
                    }

                    // Add metadata explaining this is a relogin during the session
                    QJsonObject eventData;
                    eventData["reason"] = "relogin_during_session";
                    eventData["original_login_time"] = todaySession->loginTime().toString(Qt::ISODate);
                    loginEvent->setEventData(eventData);

                    // Set metadata
                    loginEvent->setCreatedAt(QDateTime::currentDateTimeUtc());
                    loginEvent->setUpdatedAt(QDateTime::currentDateTimeUtc());
                    loginEvent->setCreatedBy(userId);
                    loginEvent->setUpdatedBy(userId);

                    bool eventSuccess = eventRepo->save(loginEvent);
                    delete loginEvent;

                    if (!eventSuccess) {
                        LOG_WARNING(QString("Failed to create additional login event for session %1")
                                  .arg(todaySession->id().toString()));
                        // Continue anyway, as the original login event exists
                    }
                }
            }

            resultSession = todaySession;
        }
        else {
            // No session for today - create a new one
            LOG_INFO("Creating new session");
            SessionModel* newSession = new SessionModel();
            // newSession->setId(QUuid::createUuid());
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

                    // Update the previous session to link to this one using direct SQL
                    QMap<QString, QVariant> contParams;
                    contParams["id"] = lastActiveSessionId.toString(QUuid::WithoutBraces);
                    contParams["continued_by"] = newSession->id().toString(QUuid::WithoutBraces);
                    contParams["updated_at"] = currentDateTime.toString(Qt::ISODate);

                    QString contQuery =
                        "UPDATE sessions SET "
                        "continued_by_session = :continued_by, "
                        "updated_at = :updated_at "
                        "WHERE id = :id";

                    if (!executeModificationQuery(contQuery, contParams)) {
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

            // Verify all required fields are set before saving
            if (!newSession->userId().isNull()
                && !newSession->machineId().isNull()
                && newSession->loginTime().isValid()) {

                LOG_DEBUG(QString("New session validation passed: login_time=%1")
                    .arg(newSession->loginTime().toString(Qt::ISODate)));

                // Save the session
                if (!save(newSession)) {
                    LOG_ERROR("Failed to save new session");
                    delete newSession;
                    return false;
                }

                // Create login event for the new session
                SessionEventRepository* eventRepo = nullptr;
                if (m_sessionEventRepository) {
                    eventRepo = m_sessionEventRepository;
                }
                if (eventRepo && eventRepo->isInitialized()) {
                    bool eventSuccess = createSessionLoginEvent(
                        newSession->id(),
                        userId,
                        machineId,
                        currentDateTime,
                        eventRepo,
                        isRemote,
                        terminalSessionId
                    );

                    if (!eventSuccess) {
                        LOG_ERROR(QString("Failed to create login event for new session %1")
                            .arg(newSession->id().toString()));
                        // We'll attempt to create this outside the transaction if needed
                    }
                }
            } else {
                LOG_ERROR("New session has missing required fields");
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

    // If we have a session but no login event was created (possibly due to transaction issues),
    // create the login event outside the transaction as a fallback
    if (resultSession && m_sessionEventRepository && m_sessionEventRepository->isInitialized()) {
        if (!hasLoginEvent(resultSession->id(), m_sessionEventRepository)) {
            LOG_WARNING(QString("No login event found for session %1 after transaction. Creating one now as fallback.")
                .arg(resultSession->id().toString()));

            createSessionLoginEvent(
                resultSession->id(),
                userId,
                machineId,
                resultSession->loginTime(),  // Use the session's login time
                m_sessionEventRepository,
                isRemote,
                terminalSessionId
            );
        }
    }

    return resultSession;
}

bool SessionRepository::safeEndSession(const QUuid &sessionId, const QDateTime &logoutTime, SessionEventRepository* eventRepository)
{
    LOG_DEBUG(QString("Safely ending session with ID: %1").arg(sessionId.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot end session: Repository not initialized");
        return false;
    }

    // Begin transaction for consistency
    bool useTransaction = eventRepository && eventRepository->isInitialized();
    if (useTransaction) {
        if (!beginTransaction()) {
            LOG_ERROR("Failed to start transaction for ending session");
            return false;
        }
    }

    // Retrieve the session first to get userId and machineId for the event
    QSharedPointer<SessionModel> session = getById(sessionId);
    if (!session) {
        LOG_ERROR(QString("Cannot find session to end: %1").arg(sessionId.toString()));
        if (useTransaction) rollbackTransaction();
        return false;
    }

    // Log the session before update
    LOG_DEBUG(QString("Session before ending: %1").arg(session->debugInfo()));

    // Check if we already have a logout event that matches this time
    bool hasExistingLogout = false;
    if (useTransaction) {
        hasExistingLogout = hasLogoutEvent(sessionId, logoutTime, eventRepository);
        LOG_DEBUG(QString("Session %1 has existing logout event at time %2: %3")
                 .arg(sessionId.toString(), logoutTime.toString(Qt::ISODate), hasExistingLogout ? "yes" : "no"));
    }

    // Use direct SQL update to avoid potential issues with model state
    QMap<QString, QVariant> params;
    params["id"] = sessionId.toString(QUuid::WithoutBraces);
    params["logout_time"] = logoutTime.toString(Qt::ISODate);
    params["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QString query =
        "UPDATE sessions SET "
        "logout_time = :logout_time, "
        "updated_at = :updated_at "
        "WHERE id = :id";

    bool success = executeModificationQuery(query, params);

    if (!success) {
        LOG_ERROR(QString("Failed to safely end session: %1, error: %2")
                  .arg(sessionId.toString(), lastError()));
        if (useTransaction) rollbackTransaction();
        return false;
    }

    // Create a logout event if event repository is available and we don't already have one
    if (useTransaction && !hasExistingLogout) {
        SessionEventModel* event = new SessionEventModel();
        event->setId(QUuid::createUuid());
        event->setSessionId(sessionId);
        event->setEventType(EventTypes::SessionEventType::Logout);
        event->setEventTime(logoutTime);
        event->setUserId(session->userId());
        event->setMachineId(session->machineId());

        // Set event metadata
        event->setCreatedAt(QDateTime::currentDateTimeUtc());
        event->setUpdatedAt(QDateTime::currentDateTimeUtc());
        event->setCreatedBy(session->userId()); // Using the session's user ID
        event->setUpdatedBy(session->userId());

        bool eventSuccess = eventRepository->save(event);
        delete event;

        if (!eventSuccess) {
            LOG_WARNING(QString("Failed to record logout event for session: %1").arg(sessionId.toString()));
            rollbackTransaction();
            return false;
        } else {
            LOG_INFO(QString("Logout event recorded for session: %1").arg(sessionId.toString()));
        }
    } else if (useTransaction && hasExistingLogout) {
        LOG_INFO(QString("Logout event already exists for session: %1, skipping creation").arg(sessionId.toString()));
    }

    if (useTransaction) {
        if (!commitTransaction()) {
            LOG_ERROR("Failed to commit transaction for ending session");
            rollbackTransaction();
            return false;
        }
    }

    // Re-fetch the session to confirm update
    auto updatedSession = getById(sessionId);
    if (updatedSession) {
        LOG_DEBUG(QString("Session after ending: %1").arg(updatedSession->debugInfo()));
    }

    LOG_INFO(QString("Session safely ended: %1").arg(sessionId.toString()));
    return true;
}

bool SessionRepository::safeReopenSession(const QUuid &sessionId, const QDateTime &updateTime, SessionEventRepository* eventRepository)
{
    LOG_DEBUG(QString("Safely reopening session with ID: %1").arg(sessionId.toString()));

    if (!isInitialized()) {
        LOG_ERROR("Cannot reopen session: Repository not initialized");
        return false;
    }

    // Begin transaction for consistency
    bool useTransaction = eventRepository && eventRepository->isInitialized();
    if (useTransaction) {
        if (!beginTransaction()) {
            LOG_ERROR("Failed to start transaction for reopening session");
            return false;
        }
    }

    // Retrieve the session first to get userId and machineId for the event
    QSharedPointer<SessionModel> session = getById(sessionId);
    if (!session) {
        LOG_ERROR(QString("Cannot find session to reopen: %1").arg(sessionId.toString()));
        if (useTransaction) rollbackTransaction();
        return false;
    }

    // Log the session before update
    LOG_DEBUG(QString("Session before reopening: %1").arg(session->debugInfo()));

    // Ensure login_time is valid before proceeding
    if (!session->loginTime().isValid()) {
        LOG_ERROR(QString("Session has invalid login time: %1").arg(sessionId.toString()));
        if (useTransaction) rollbackTransaction();
        return false;
    }

    // Check if there's a valid logout time but no matching logout event
    if (useTransaction && session->logoutTime().isValid()) {
        // Check if there's already a logout event matching this logout time
        bool hasMatchingLogoutEvent = hasLogoutEvent(sessionId, session->logoutTime(), eventRepository);

        if (!hasMatchingLogoutEvent) {
            LOG_INFO(QString("Creating missing logout event for session %1 at time %2")
                     .arg(sessionId.toString(), session->logoutTime().toString(Qt::ISODate)));

            SessionEventModel* logoutEvent = new SessionEventModel();
            logoutEvent->setId(QUuid::createUuid());
            logoutEvent->setSessionId(sessionId);
            logoutEvent->setEventType(EventTypes::SessionEventType::Logout);
            logoutEvent->setEventTime(session->logoutTime());
            logoutEvent->setUserId(session->userId());
            logoutEvent->setMachineId(session->machineId());

            // Add context that this is an auto-generated logout
            QJsonObject logoutData;
            logoutData["reason"] = "auto_generated_for_consistency";
            logoutData["auto_generated"] = true;
            logoutEvent->setEventData(logoutData);

            // Set metadata
            logoutEvent->setCreatedAt(QDateTime::currentDateTimeUtc());
            logoutEvent->setUpdatedAt(QDateTime::currentDateTimeUtc());
            logoutEvent->setCreatedBy(session->userId());
            logoutEvent->setUpdatedBy(session->userId());

            bool logoutSuccess = eventRepository->save(logoutEvent);
            delete logoutEvent;

            if (!logoutSuccess) {
                LOG_WARNING(QString("Failed to create missing logout event for session: %1").arg(sessionId.toString()));
                // Continue anyway - this is not critical enough to fail the whole operation
            } else {
                LOG_INFO(QString("Missing logout event created for session: %1").arg(sessionId.toString()));
            }
        }
    }

    // Use direct SQL update to avoid potential issues with model state
    QMap<QString, QVariant> params;
    params["id"] = sessionId.toString(QUuid::WithoutBraces);
    params["updated_at"] = updateTime.toString(Qt::ISODate);

    QString query =
        "UPDATE sessions SET "
        "logout_time = NULL, "
        "updated_at = :updated_at "
        "WHERE id = :id";

    bool success = executeModificationQuery(query, params);

    if (!success) {
        LOG_ERROR(QString("Failed to safely reopen session: %1, error: %2")
                  .arg(sessionId.toString(), lastError()));
        if (useTransaction) rollbackTransaction();
        return false;
    }

    // Create a login event if repository is available
    if (useTransaction) {
        // Verify if there's already a login event for this reopening
        bool hasRecentLoginEvent = hasLoginEvent(sessionId, updateTime, eventRepository);

        // Only create a new login event if one doesn't already exist
        if (!hasRecentLoginEvent) {
            SessionEventModel* event = new SessionEventModel();
            event->setId(QUuid::createUuid());
            event->setSessionId(sessionId);
            event->setEventType(EventTypes::SessionEventType::Login);
            event->setEventTime(updateTime);  // Use current time for the event
            event->setUserId(session->userId());
            event->setMachineId(session->machineId());

            // Add additional context that this is a reopened session
            QJsonObject eventData;
            eventData["reason"] = "session_reopened";
            eventData["auto_generated"] = true;
            eventData["original_login_time"] = session->loginTime().toString(Qt::ISODate);
            event->setEventData(eventData);

            // Set event metadata
            event->setCreatedAt(QDateTime::currentDateTimeUtc());
            event->setUpdatedAt(QDateTime::currentDateTimeUtc());
            event->setCreatedBy(session->userId());
            event->setUpdatedBy(session->userId());

            bool eventSuccess = eventRepository->save(event);
            delete event;

            if (!eventSuccess) {
                LOG_WARNING(QString("Failed to record login event for reopened session: %1").arg(sessionId.toString()));
                rollbackTransaction();
                return false;
            } else {
                LOG_INFO(QString("Login event recorded for reopened session: %1").arg(sessionId.toString()));
            }
        } else {
            LOG_INFO(QString("Recent login event already exists for session: %1").arg(sessionId.toString()));
        }
    }

    if (useTransaction) {
        if (!commitTransaction()) {
            LOG_ERROR("Failed to commit transaction for reopening session");
            rollbackTransaction();
            return false;
        }
    }

    // Re-fetch the session to confirm update
    auto updatedSession = getById(sessionId);
    if (updatedSession) {
        LOG_DEBUG(QString("Session after reopening: %1").arg(updatedSession->debugInfo()));
    }

    LOG_INFO(QString("Session safely reopened: %1").arg(sessionId.toString()));
    return true;
}

bool SessionRepository::createSessionLoginEvent(
    const QUuid &sessionId,
    const QUuid &userId,
    const QUuid &machineId,
    const QDateTime &loginTime,
    SessionEventRepository* eventRepository,
    bool isRemote,
    const QString& terminalSessionId)
{
    if (!eventRepository || !eventRepository->isInitialized()) {
        LOG_WARNING("Cannot create login event - SessionEventRepository not available");
        return false;
    }

    LOG_DEBUG(QString("Creating login event for session: %1 at time: %2")
              .arg(sessionId.toString(), loginTime.toString(Qt::ISODate)));

    // Check for existing login events at this time to avoid duplicates
    bool hasExistingLoginEvent = hasLoginEvent(sessionId, loginTime, eventRepository);

    if (hasExistingLoginEvent) {
        LOG_INFO(QString("Login event already exists for session %1 at time %2, skipping creation")
                .arg(sessionId.toString(), loginTime.toString(Qt::ISODate)));
        return true; // Already exists, no need to create
    }

    // Create new event with detailed logging
    LOG_INFO("===== CREATING SESSION LOGIN EVENT =====");
    LOG_INFO(QString("Session ID: %1").arg(sessionId.toString()));
    LOG_INFO(QString("User ID: %1").arg(userId.toString()));
    LOG_INFO(QString("Machine ID: %1").arg(machineId.toString()));
    LOG_INFO(QString("Login Time: %1").arg(loginTime.toString(Qt::ISODate)));
    LOG_INFO(QString("Is Remote: %1").arg(isRemote ? "true" : "false"));
    LOG_INFO(QString("Terminal Session ID: %1").arg(terminalSessionId.isEmpty() ? "none" : terminalSessionId));

    // Check if session exists in database
    if (!exists(sessionId)) {
        LOG_ERROR(QString("Cannot create login event - Session %1 does NOT exist in database!").arg(sessionId.toString()));
        return false;
    } else {
        LOG_INFO(QString("Session %1 exists in database").arg(sessionId.toString()));
    }

    // Create retry loop for event creation
    const int MAX_RETRIES = 3;
    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        if (attempt > 1) {
            LOG_INFO(QString("Retry attempt %1 of %2").arg(attempt).arg(MAX_RETRIES));
            // Brief pause before retry
            QThread::msleep(100 * attempt);
        }

        SessionEventModel* event = new SessionEventModel();
        event->setId(QUuid::createUuid());
        LOG_INFO(QString("Generated Event ID: %1").arg(event->id().toString()));

        event->setSessionId(sessionId);
        event->setEventType(EventTypes::SessionEventType::Login);
        event->setEventTime(loginTime);
        event->setUserId(userId);
        event->setMachineId(machineId);
        event->setIsRemote(isRemote);

        if (!terminalSessionId.isEmpty()) {
            event->setTerminalSessionId(terminalSessionId);
        }

        // Add metadata
        QDateTime now = QDateTime::currentDateTimeUtc();
        event->setCreatedBy(userId);
        event->setUpdatedBy(userId);
        event->setCreatedAt(now);
        event->setUpdatedAt(now);

        // Now try to save the event
        bool success = eventRepository->save(event);

        if (success) {
            LOG_INFO(QString("Login event created successfully: %1 (attempt %2)")
                     .arg(event->id().toString())
                     .arg(attempt));
            delete event;
            return true;
        } else {
            LOG_ERROR(QString("Failed to create login event! Error: %1 (attempt %2)")
                     .arg(eventRepository->lastError())
                     .arg(attempt));

            // Additional debugging for specific cases
            if (eventRepository->lastError().contains("constraint", Qt::CaseInsensitive)) {
                LOG_ERROR("Failure appears to be a constraint violation");
            }
            if (eventRepository->lastError().contains("foreign key", Qt::CaseInsensitive)) {
                LOG_ERROR("Failure appears to be a foreign key constraint");
            }
            if (eventRepository->lastError().contains("null", Qt::CaseInsensitive)) {
                LOG_ERROR("Failure appears to involve NULL values");
            }

            delete event;

            // Continue to retry unless this was the last attempt
            if (attempt == MAX_RETRIES) {
                LOG_ERROR("All retry attempts failed");
                break;
            }
        }
    }

    LOG_INFO("=======================================");
    return false;
}

bool SessionRepository::hasLoginEvent(const QUuid &sessionId, SessionEventRepository* eventRepository)
{
    return hasLoginEvent(sessionId, QDateTime(), eventRepository);
}

bool SessionRepository::hasLoginEvent(const QUuid &sessionId, const QDateTime &loginTime, SessionEventRepository* eventRepository)
{
    if (!eventRepository || !eventRepository->isInitialized()) {
        LOG_WARNING("Cannot check for login events - SessionEventRepository not available");
        return false;
    }

    LOG_DEBUG(QString("Checking for login events for session: %1%2")
              .arg(sessionId.toString())
              .arg(loginTime.isValid() ?
                   QString(" at specific time: %1").arg(loginTime.toString(Qt::ISODate)) :
                   " at any time"));

    // Get all events for this session
    // Limit results to improve performance - we likely don't need to check thousands of events
    QList<QSharedPointer<SessionEventModel>> events = eventRepository->getBySessionId(sessionId, 100);

    // The time tolerance (in seconds) for timestamp comparison
    const int TIME_TOLERANCE_SECONDS = 60;

    // Check if any of these events are login events matching the requested time
    bool hasMatchingLoginEvent = false;

    for (const auto& event : events) {
        if (event->eventType() == EventTypes::SessionEventType::Login) {
            if (loginTime.isValid()) {
                // Check if the event time is close to the requested login time
                int timeDiff = qAbs(event->eventTime().secsTo(loginTime));
                if (timeDiff <= TIME_TOLERANCE_SECONDS) {
                    hasMatchingLoginEvent = true;
                    break;
                }
            } else {
                // If no specific time requested, just check if any login event exists
                hasMatchingLoginEvent = true;
                break;
            }
        }
    }

    LOG_DEBUG(QString("Session %1 has %2login event%3")
              .arg(sessionId.toString())
              .arg(hasMatchingLoginEvent ? "" : "no ")
              .arg(loginTime.isValid() ?
                   QString(" around time %1").arg(loginTime.toString(Qt::ISODate)) :
                   ""));

    return hasMatchingLoginEvent;
}

bool SessionRepository::hasLogoutEvent(const QUuid &sessionId, SessionEventRepository* eventRepository)
{
    return hasLogoutEvent(sessionId, QDateTime(), eventRepository);
}

bool SessionRepository::hasLogoutEvent(const QUuid &sessionId, const QDateTime &logoutTime, SessionEventRepository* eventRepository)
{
    if (!eventRepository || !eventRepository->isInitialized()) {
        LOG_WARNING("Cannot check for logout events - SessionEventRepository not available");
        return false;
    }

    LOG_DEBUG(QString("Checking for logout events for session: %1 at time: %2")
              .arg(sessionId.toString(), logoutTime.toString(Qt::ISODate)));

    // Query the events repository directly for events with this session ID
    QList<QSharedPointer<SessionEventModel>> events = eventRepository->getBySessionId(sessionId);

    // Check if any of these events are logout events matching the time
    bool hasMatchingLogoutEvent = false;

    // Use a small time tolerance (e.g., 5 seconds) for comparison
    const int timeTolerance = 5; // in seconds

    for (const auto& event : events) {
        if (event->eventType() == EventTypes::SessionEventType::Logout) {
            if (logoutTime.isValid()) {
                // Check if the event time is close to the requested logout time
                int timeDiff = qAbs(event->eventTime().secsTo(logoutTime));
                if (timeDiff <= timeTolerance) {
                    hasMatchingLogoutEvent = true;
                    break;
                }
            } else {
                // If no specific time requested, just check if any logout event exists
                hasMatchingLogoutEvent = true;
                break;
            }
        }
    }

    LOG_DEBUG(QString("Session %1 has %2logout event %3")
              .arg(sessionId.toString())
              .arg(hasMatchingLogoutEvent ? "" : "no ")
              .arg(logoutTime.isValid() ? QString("at time %1").arg(logoutTime.toString(Qt::ISODate)) : ""));

    return hasMatchingLogoutEvent;
}

bool SessionRepository::verifyLoginLogoutPairs(const QUuid &sessionId, SessionEventRepository* eventRepository)
{
    if (!eventRepository || !eventRepository->isInitialized()) {
        LOG_WARNING("Cannot verify login/logout pairs - SessionEventRepository not available");
        return false;
    }

    LOG_DEBUG(QString("Verifying login/logout pairs for session: %1").arg(sessionId.toString()));

    // Get all events for this session
    QList<QSharedPointer<SessionEventModel>> events = eventRepository->getBySessionId(sessionId);

    // Sort events by timestamp to ensure proper ordering
    std::sort(events.begin(), events.end(), [](const QSharedPointer<SessionEventModel>& a, const QSharedPointer<SessionEventModel>& b) {
        return a->eventTime() < b->eventTime();
    });

    // Track the login/logout sequence
    bool currentlyLoggedIn = false;
    int unpaired = 0;

    for (const auto& event : events) {
        if (event->eventType() == EventTypes::SessionEventType::Login) {
            if (currentlyLoggedIn) {
                // Found a login while already logged in - this is unpaired
                unpaired++;
                LOG_WARNING(QString("Unpaired login event found at %1 for session %2")
                          .arg(event->eventTime().toString(Qt::ISODate), sessionId.toString()));
            }
            currentlyLoggedIn = true;
        }
        else if (event->eventType() == EventTypes::SessionEventType::Logout) {
            if (!currentlyLoggedIn) {
                // Found a logout without being logged in - this is unpaired
                unpaired++;
                LOG_WARNING(QString("Unpaired logout event found at %1 for session %2")
                          .arg(event->eventTime().toString(Qt::ISODate), sessionId.toString()));
            }
            currentlyLoggedIn = false;
        }
    }

    // It's okay to end with currentlyLoggedIn = true (open session)
    // but we want to track any other inconsistencies
    bool consistent = (unpaired == 0);

    LOG_INFO(QString("Login/logout verification for session %1: %2 (unpaired events: %3)")
             .arg(sessionId.toString())
             .arg(consistent ? "consistent" : "inconsistent")
             .arg(unpaired));

    return consistent;
}
