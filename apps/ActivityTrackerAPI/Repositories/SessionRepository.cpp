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
    , m_sessionEventRepository(nullptr)
{
    LOG_DEBUG("SessionRepository created");
}

void SessionRepository::setSessionEventRepository(SessionEventRepository* sessionEventRepository) {
    m_sessionEventRepository = sessionEventRepository;
    LOG_DEBUG("SessionEventRepository set in SessionRepository");
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
           "(user_id, login_time, logout_time, machine_id, "
           "session_data, created_at, created_by, updated_at, updated_by, continued_from_session, "
           "continued_by_session, previous_session_end_time, time_since_previous_session) "
           "VALUES "
           "(:user_id, :login_time, :logout_time, :machine_id, "
           ":session_data, :created_at, :created_by, :updated_at, :updated_by, "
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
    params["session_data"] = QString::fromUtf8(QJsonDocument(session->sessionData()).toJson());
    params["login_time"] = session->loginTime().toUTC();
    params["created_at"] = session->createdAt().toUTC();
    params["updated_at"] = session->updatedAt().toUTC();
    params["time_since_previous_session"] = QString::number(session->timeSincePreviousSession());

    // For nullable UUID fields - use QVariant::Invalid for NULL
    params["created_by"] = session->createdBy().isNull() ? QVariant(QVariant::Invalid) : session->createdBy().toString(QUuid::WithoutBraces);
    params["updated_by"] = session->updatedBy().isNull() ? QVariant(QVariant::Invalid) : session->updatedBy().toString(QUuid::WithoutBraces);
    params["continued_from_session"] = session->continuedFromSession().isNull() ? QVariant(QVariant::Invalid) : session->continuedFromSession().toString(QUuid::WithoutBraces);
    params["continued_by_session"] = session->continuedBySession().isNull() ? QVariant(QVariant::Invalid) : session->continuedBySession().toString(QUuid::WithoutBraces);

    // For nullable timestamp fields
    params["logout_time"] = session->logoutTime().isValid() ? session->logoutTime().toUTC() : QVariant(QVariant::Invalid);
    params["previous_session_end_time"] = session->previousSessionEndTime().isValid() ? session->previousSessionEndTime().toUTC() : QVariant(QVariant::Invalid);

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
            .arg((*result)->loginTime().toUTC().toString())
            .arg((*result)->logoutTime().isValid() ? (*result)->logoutTime().toUTC().toString() : "NULL"));

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
        params1["updated_at"] = QDateTime::currentDateTimeUtc().toUTC();

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
        params2["previous_session_end_time"] = previousSession->logoutTime().toUTC();
        params2["time_since_previous_session"] = QString::number(previousSession->logoutTime().secsTo(newSession->loginTime()));
        params2["updated_at"] = QDateTime::currentDateTimeUtc().toUTC();

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
            data["first_login"] = chainStats->firstLogin.toUTC().toString();
            data["last_activity"] = chainStats->lastActivity.toUTC().toString();
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
              .arg(startDate.toUTC().toString())
              .arg(endDate.toUTC().toString()));

    QJsonObject stats;

    if (!isInitialized()) {
        LOG_ERROR("Cannot get user session stats: Repository not initialized");
        return stats;
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["start_date"] = startDate.toUTC();
    params["end_date"] = endDate.toUTC();

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
            data["first_login"] = userStats->firstLogin.toUTC().toString();
            data["last_activity"] = userStats->lastActivity.toUTC().toString();
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
    afkParams["start_date"] = startDate.toUTC();
    afkParams["end_date"] = endDate.toUTC();

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
        .arg(date.toString()));

    if (!ensureInitialized()) {
        return nullptr;
    }

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["machine_id"] = machineId.toString(QUuid::WithoutBraces);
    params["start_of_day"] = QDateTime(date, QTime(0, 0, 0), Qt::UTC);
    params["end_of_day"] = QDateTime(date, QTime(23, 59, 59, 999), Qt::UTC);

    QString query =
        "SELECT * FROM sessions WHERE user_id = :user_id AND machine_id = :machine_id "
        "AND login_time >= :start_of_day AND login_time <= :end_of_day "
        "ORDER BY login_time DESC LIMIT 1";

    auto result = executeSingleSelectQuery(query, params);

    if (result) {
        LOG_INFO(QString("Found session for user ID: %1, machine ID: %2, date: %3 - Session ID: %4")
            .arg(userId.toString())
            .arg(machineId.toString())
            .arg(date.toString())
            .arg(result->id().toString()));
    }
    else {
        LOG_INFO(QString("No session found for user ID: %1, machine ID: %2, date: %3")
            .arg(userId.toString())
            .arg(machineId.toString())
            .arg(date.toString()));
    }

    return result;
}

QSharedPointer<SessionModel> SessionRepository::createOrReuseSessionWithTransaction(
    const QUuid& userId,
    const QUuid& machineId,
    const QDateTime& currentDateTime,
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

    // Get or create the session for today
    QSharedPointer<SessionModel> resultSession = getOrCreateSessionForToday(
        userId, machineId, currentDateTime, sessionData);

    if (!resultSession) {
        LOG_ERROR("Failed to get or create session");
        return nullptr;
    }

    // Create necessary events
    if (m_sessionEventRepository && m_sessionEventRepository->isInitialized()) {
        createSessionEvents(resultSession->id(), userId, machineId, currentDateTime, isRemote, terminalSessionId);
    }

    return resultSession;
}

QSharedPointer<SessionModel> SessionRepository::getOrCreateSessionForToday(
    const QUuid& userId,
    const QUuid& machineId,
    const QDateTime& currentDateTime,
    const QJsonObject& sessionData)
{
    QSharedPointer<SessionModel> resultSession;
    QDate currentDate = currentDateTime.date();

    // Find session for today
    auto currentDaySession = findSessionForDay(userId, machineId, currentDate);

    // Execute the session creation/update in a transaction
    bool success = executeInTransaction([&]() {
        if (currentDaySession) {
            LOG_INFO(QString("Found session for current day: %1").arg(currentDaySession->id().toString()));

            // If session is closed (has logout time), reopen it
            if (currentDaySession->logoutTime().isValid()) {
                LOG_INFO(QString("Reopening closed session: %1").arg(currentDaySession->id().toString()));

                currentDaySession->setLogoutTime(QDateTime()); // Clear logout time
                currentDaySession->setUpdatedAt(currentDateTime);
                currentDaySession->setUpdatedBy(userId);

                if (!update(currentDaySession.data())) {
                    LOG_ERROR(QString("Failed to reopen session: %1").arg(currentDaySession->id().toString()));
                    return false;
                }
            }

            resultSession = currentDaySession;
            return true;
        } else {
            // End any active session from a previous day
            endPreviousDaySession(userId, machineId, currentDateTime);

            // Create a new session for today
            LOG_INFO("Creating new session for today");
            SessionModel* newSession = new SessionModel();
            newSession->setUserId(userId);
            newSession->setLoginTime(currentDateTime);
            newSession->setMachineId(machineId);
            newSession->setSessionData(sessionData);

            // Set metadata
            newSession->setCreatedBy(userId);
            newSession->setUpdatedBy(userId);
            newSession->setCreatedAt(currentDateTime);
            newSession->setUpdatedAt(currentDateTime);

            if (!save(newSession)) {
                LOG_ERROR("Failed to save new session");
                delete newSession;
                return false;
            }

            resultSession = QSharedPointer<SessionModel>(newSession);
            return true;
        }
    });

    return success ? resultSession : nullptr;
}

QSharedPointer<SessionModel> SessionRepository::findSessionForDay(
    const QUuid& userId,
    const QUuid& machineId,
    const QDate& date)
{
    LOG_DEBUG(QString("Finding session for date: %1").arg(date.toString()));

    QMap<QString, QVariant> params;
    params["user_id"] = userId.toString(QUuid::WithoutBraces);
    params["machine_id"] = machineId.toString(QUuid::WithoutBraces);
    params["start_of_day"] = QDateTime(date, QTime(0, 0, 0), Qt::UTC).toUTC();
    params["end_of_day"] = QDateTime(date, QTime(23, 59, 59, 999), Qt::UTC).toUTC();

    QString query =
        "SELECT * FROM sessions WHERE user_id = :user_id AND machine_id = :machine_id "
        "AND login_time >= :start_of_day AND login_time <= :end_of_day "
        "ORDER BY login_time DESC LIMIT 1";

    return executeSingleSelectQuery(query, params);
}

bool SessionRepository::endPreviousDaySession(
    const QUuid& userId,
    const QUuid& machineId,
    const QDateTime& currentDateTime)
{
    LOG_DEBUG("Checking for active sessions from previous days");

    auto activeSession = getActiveSessionForUser(userId, machineId);
    if (activeSession && activeSession->loginTime().date() != currentDateTime.date()) {
        LOG_INFO(QString("Found active session from previous day: %1, closing it")
            .arg(activeSession->id().toString()));

        // Close the session
        activeSession->setLogoutTime(currentDateTime);
        activeSession->setUpdatedAt(currentDateTime);
        activeSession->setUpdatedBy(userId);

        if (!update(activeSession.data())) {
            LOG_ERROR(QString("Failed to end previous day's session: %1")
                .arg(activeSession->id().toString()));
            return false;
        }

        // Create logout event will happen in createSessionEvents
        return true;
    }

    return true; // No previous day session to end
}

bool SessionRepository::createSessionEvents(
    const QUuid& sessionId,
    const QUuid& userId,
    const QUuid& machineId,
    const QDateTime& currentDateTime,
    bool isRemote,
    const QString& terminalSessionId)
{
    LOG_DEBUG(QString("Creating session events for session: %1").arg(sessionId.toString()));

    if (!m_sessionEventRepository || !m_sessionEventRepository->isInitialized()) {
        LOG_WARNING("Session event repository not available or not initialized");
        return false;
    }

    return executeInTransaction([&]() {
        // First verify the session exists in the database
        if (!exists(sessionId)) {
            LOG_ERROR(QString("Session %1 doesn't exist in database!")
                .arg(sessionId.toString()));
            return false;
        }

        // Get all session events sorted by time
        QList<QSharedPointer<SessionEventModel>> sessionEvents = m_sessionEventRepository->getBySessionId(sessionId);

        // Check if we need to create a logout before login
        bool needLogout = checkIfLogoutNeeded(sessionEvents);
        QDateTime lastLoginTime;

        if (needLogout && !sessionEvents.isEmpty()) {
            // Find the last login time
            for (const auto& event : sessionEvents) {
                if (event->eventType() == EventTypes::SessionEventType::Login) {
                    lastLoginTime = event->eventTime();
                    break;
                }
            }

            // Create logout event
            if (!createLogoutEvent(
                sessionId, userId, machineId, lastLoginTime,
                currentDateTime, isRemote)) {
                LOG_ERROR("Failed to create logout event");
                // Continue anyway to try creating login
            }
        }

        // Create login event (unless one exists at almost exactly this time)
        if (!loginEventExistsAtTime(sessionEvents, currentDateTime)) {
            if (!createLoginEvent(
                sessionId, userId, machineId, currentDateTime,
                isRemote, terminalSessionId, needLogout)) {
                LOG_ERROR("Failed to create login event");
                return false;
            }
        } else {
            LOG_INFO(QString("Login event already exists at time %1, skipping creation")
                .arg(currentDateTime.toUTC().toString()));
        }

        return true;
    });
}

bool SessionRepository::checkIfLogoutNeeded(
    const QList<QSharedPointer<SessionEventModel>>& events)
{
    if (events.isEmpty()) {
        return false;
    }

    // Sort events by time (newest first)
    QList<QSharedPointer<SessionEventModel>> sortedEvents = events;
    std::sort(sortedEvents.begin(), sortedEvents.end(),
        [](const QSharedPointer<SessionEventModel>& a, const QSharedPointer<SessionEventModel>& b) {
            return a->eventTime() > b->eventTime();
        });

    // If the most recent event is a login, we need a logout
    if (!sortedEvents.isEmpty() &&
        sortedEvents.first()->eventType() == EventTypes::SessionEventType::Login) {
        LOG_INFO(QString("Last event is a login at %1, need to create logout first")
            .arg(sortedEvents.first()->eventTime().toUTC().toString()));
        return true;
    }

    LOG_INFO("Last event is not a login, we can create a new login directly");
    return false;
}

bool SessionRepository::loginEventExistsAtTime(
    const QList<QSharedPointer<SessionEventModel>>& events,
    const QDateTime& time)
{
    // Check if there's a login event within 5 seconds of the given time
    for (const auto& event : events) {
        if (event->eventType() == EventTypes::SessionEventType::Login &&
            qAbs(event->eventTime().secsTo(time)) < 5) { // within 5 seconds
            return true;
        }
    }

    return false;
}

bool SessionRepository::createLogoutEvent(
    const QUuid& sessionId,
    const QUuid& userId,
    const QUuid& machineId,
    const QDateTime& lastLoginTime,
    const QDateTime& currentTime,
    bool isRemote)
{
    LOG_INFO(QString("Creating logout event for session %1").arg(sessionId.toString()));

    // Create a logout event
    SessionEventModel* logoutEvent = new SessionEventModel();
    logoutEvent->setId(QUuid::createUuid());
    logoutEvent->setSessionId(sessionId);
    logoutEvent->setEventType(EventTypes::SessionEventType::Logout);

    // Place logout just before the current time
    QDateTime logoutTime = currentTime.addSecs(-30);
    if (logoutTime < lastLoginTime) {
        // Ensure logout is after login
        logoutTime = lastLoginTime.addSecs(30);
    }

    logoutEvent->setEventTime(logoutTime);
    logoutEvent->setUserId(userId);
    logoutEvent->setMachineId(machineId);
    logoutEvent->setIsRemote(isRemote);

    QJsonObject logoutData;
    logoutData["reason"] = "auto_generated_before_new_login";
    logoutData["auto_generated"] = true;
    logoutEvent->setEventData(logoutData);

    QDateTime now = QDateTime::currentDateTimeUtc();
    logoutEvent->setCreatedBy(userId);
    logoutEvent->setUpdatedBy(userId);
    logoutEvent->setCreatedAt(now);
    logoutEvent->setUpdatedAt(now);

    bool success = m_sessionEventRepository->save(logoutEvent);
    delete logoutEvent;

    if (success) {
        LOG_INFO(QString("Successfully created logout event at %1 for session %2")
            .arg(logoutTime.toUTC().toString(), sessionId.toString()));
    } else {
        LOG_ERROR(QString("Failed to create logout event for session %1")
            .arg(sessionId.toString()));
    }

    return success;
}

bool SessionRepository::createLoginEvent(
    const QUuid& sessionId,
    const QUuid& userId,
    const QUuid& machineId,
    const QDateTime& loginTime,
    bool isRemote,
    const QString& terminalSessionId,
    bool afterLogout)
{
    LOG_INFO(QString("Creating login event for session %1").arg(sessionId.toString()));

    SessionEventModel* loginEvent = new SessionEventModel();
    loginEvent->setId(QUuid::createUuid());
    loginEvent->setSessionId(sessionId);
    loginEvent->setEventType(EventTypes::SessionEventType::Login);
    loginEvent->setEventTime(loginTime);
    loginEvent->setUserId(userId);
    loginEvent->setMachineId(machineId);
    loginEvent->setIsRemote(isRemote);

    if (!terminalSessionId.isEmpty()) {
        loginEvent->setTerminalSessionId(terminalSessionId);
    }

    // Add context about this login
    QJsonObject loginData;
    if (afterLogout) {
        loginData["reason"] = "new_login_after_closing_previous";
    } else {
        loginData["reason"] = "new_login";
    }
    loginEvent->setEventData(loginData);

    QDateTime now = QDateTime::currentDateTimeUtc();
    loginEvent->setCreatedBy(userId);
    loginEvent->setUpdatedBy(userId);
    loginEvent->setCreatedAt(now);
    loginEvent->setUpdatedAt(now);

    bool success = m_sessionEventRepository->save(loginEvent);
    delete loginEvent;

    if (success) {
        LOG_INFO(QString("Successfully created login event at %1 for session %2")
            .arg(loginTime.toUTC().toString(), sessionId.toString()));
    } else {
        LOG_ERROR(QString("Failed to create login event for session %1: %2")
            .arg(sessionId.toString())
            .arg(m_sessionEventRepository->lastError()));
    }

    return success;
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
                 .arg(sessionId.toString(), logoutTime.toUTC().toString(), hasExistingLogout ? "yes" : "no"));
    }

    // Use direct SQL update to avoid potential issues with model state
    QMap<QString, QVariant> params;
    params["id"] = sessionId.toString(QUuid::WithoutBraces);
    params["logout_time"] = logoutTime.toUTC();
    params["updated_at"] = QDateTime::currentDateTimeUtc().toUTC();

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
                     .arg(sessionId.toString(), session->logoutTime().toUTC().toString()));

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
    params["updated_at"] = updateTime.toUTC();

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
            eventData["original_login_time"] = session->loginTime().toUTC().toString();
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
              .arg(sessionId.toString(), loginTime.toUTC().toString()));

    // Check for existing login events at this time to avoid duplicates
    bool hasExistingLoginEvent = hasLoginEvent(sessionId, loginTime, eventRepository);

    if (hasExistingLoginEvent) {
        LOG_INFO(QString("Login event already exists for session %1 at time %2, skipping creation")
                .arg(sessionId.toString(), loginTime.toUTC().toString()));
        return true; // Already exists, no need to create
    }

    // Create new event with detailed logging
    LOG_INFO("===== CREATING SESSION LOGIN EVENT =====");
    LOG_INFO(QString("Session ID: %1").arg(sessionId.toString()));
    LOG_INFO(QString("User ID: %1").arg(userId.toString()));
    LOG_INFO(QString("Machine ID: %1").arg(machineId.toString()));
    LOG_INFO(QString("Login Time: %1").arg(loginTime.toUTC().toString()));
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
                   QString(" at specific time: %1").arg(loginTime.toUTC().toString()) :
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
                   QString(" around time %1").arg(loginTime.toUTC().toString()) :
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
              .arg(sessionId.toString(), logoutTime.toUTC().toString()));

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
              .arg(logoutTime.isValid() ? QString("at time %1").arg(logoutTime.toUTC().toString()) : ""));

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
                          .arg(event->eventTime().toUTC().toString(), sessionId.toString()));
            }
            currentlyLoggedIn = true;
        }
        else if (event->eventType() == EventTypes::SessionEventType::Logout) {
            if (!currentlyLoggedIn) {
                // Found a logout without being logged in - this is unpaired
                unpaired++;
                LOG_WARNING(QString("Unpaired logout event found at %1 for session %2")
                          .arg(event->eventTime().toUTC().toString(), sessionId.toString()));
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
