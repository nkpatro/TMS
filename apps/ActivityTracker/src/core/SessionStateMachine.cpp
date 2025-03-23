#include "SessionStateMachine.h"
#include "SessionManager.h"
#include "logger/logger.h"
#include <QJsonObject>

SessionStateMachine::SessionStateMachine(SessionManager* sessionManager, QObject *parent)
    : QObject(parent)
    , m_sessionManager(sessionManager)
    , m_currentState(State::Inactive)
    , m_inactiveState(nullptr)
    , m_activeState(nullptr)
    , m_afkState(nullptr)
    , m_suspendedState(nullptr)
    , m_reconnectingState(nullptr)
    , m_endingState(nullptr)
    , m_finalState(nullptr)
{
}

SessionStateMachine::~SessionStateMachine()
{
    m_stateMachine.stop();
}

bool SessionStateMachine::initialize()
{
    LOG_INFO("Initializing SessionStateMachine");
    
    // Create states
    setupStates();
    
    // Define transitions between states
    setupTransitions();
    
    // Connect state machine's finished signal to our resetStateMachine slot
    connect(&m_stateMachine, &QStateMachine::finished, this, &SessionStateMachine::resetStateMachine);

    // Start the state machine
    m_stateMachine.start();

    LOG_INFO("SessionStateMachine initialized successfully");
    return true;
}

SessionStateMachine::State SessionStateMachine::currentState() const
{
    return m_currentState;
}

QUuid SessionStateMachine::currentSessionId() const
{
    return m_currentSessionId;
}

QDateTime SessionStateMachine::sessionStartTime() const
{
    return m_sessionStartTime;
}

void SessionStateMachine::startSession(const QUuid& sessionId, const QDateTime& startTime)
{
    LOG_INFO(QString("Starting session: %1").arg(sessionId.toString()));

    m_currentSessionId = sessionId;
    m_sessionStartTime = startTime;

    emit sessionStarted();
}

void SessionStateMachine::endSession()
{
    LOG_INFO(QString("Ending session: %1").arg(m_currentSessionId.toString()));
    emit sessionEnded();
}

void SessionStateMachine::userAfk(bool isAfk)
{
    if (isAfk) {
        LOG_DEBUG("User went AFK");
        emit userWentAfk();
    } else {
        LOG_DEBUG("User returned from AFK");
        emit userReturnedFromAfk();
    }
}

void SessionStateMachine::systemSuspending()
{
    LOG_INFO("System suspending");
    emit systemSuspend();
}

void SessionStateMachine::systemResuming()
{
    LOG_INFO("System resuming");
    emit systemResume();
}

void SessionStateMachine::connectionLost()
{
    LOG_WARNING("Connection to server lost");
    emit lostConnection();
}

void SessionStateMachine::connectionRestored()
{
    LOG_INFO("Connection to server restored");
    emit restoredConnection();
}

void SessionStateMachine::resetStateMachine()
{
    LOG_INFO("State machine finished, resetting to Inactive state");

    // Clear session data
    m_currentSessionId = QUuid();
    m_sessionStartTime = QDateTime();

    // Force transition to inactive state
    m_stateMachine.setInitialState(m_inactiveState);
    m_stateMachine.stop();
    m_stateMachine.start();

    // Update current state
    transitionToState(State::Inactive);
}

void SessionStateMachine::setupStates()
{
    // Create all states
    m_inactiveState = new QState(&m_stateMachine);
    m_activeState = new QState(&m_stateMachine);
    m_afkState = new QState(&m_stateMachine);
    m_suspendedState = new QState(&m_stateMachine);
    m_reconnectingState = new QState(&m_stateMachine);
    m_endingState = new QState(&m_stateMachine);
    m_finalState = new QFinalState(&m_stateMachine);

    // Set names for debugging
    m_inactiveState->setObjectName("Inactive");
    m_activeState->setObjectName("Active");
    m_afkState->setObjectName("Afk");
    m_suspendedState->setObjectName("Suspended");
    m_reconnectingState->setObjectName("Reconnecting");
    m_endingState->setObjectName("Ending");
    m_finalState->setObjectName("Final");

    // Set initial state
    m_stateMachine.setInitialState(m_inactiveState);

    // Define entry/exit actions for states

    // Inactive state
    connect(m_inactiveState, &QState::entered, [this]() {
        LOG_INFO("Entered Inactive state");
        transitionToState(State::Inactive);
    });

    // Active state
    connect(m_activeState, &QState::entered, [this]() {
        LOG_INFO("Entered Active state");
        transitionToState(State::Active);

        // Record session activity if we have a session manager
        if (m_sessionManager && !m_currentSessionId.isNull()) {
            QJsonObject eventData;
            eventData["state"] = "active";
            m_sessionManager->recordSessionEvent(m_currentSessionId, "state_change", eventData);
        }
    });

    // AFK state
    connect(m_afkState, &QState::entered, [this]() {
        LOG_INFO("Entered AFK state");
        transitionToState(State::Afk);

        // Start AFK period tracking
        if (m_sessionManager && !m_currentSessionId.isNull()) {
            m_sessionManager->startAfkPeriod(m_currentSessionId);
        }
    });

    connect(m_afkState, &QState::exited, [this]() {
        LOG_INFO("Exited AFK state");

        // End AFK period tracking
        if (m_sessionManager && !m_currentSessionId.isNull()) {
            m_sessionManager->endAfkPeriod(m_currentSessionId);
        }
    });

    // Suspended state
    connect(m_suspendedState, &QState::entered, [this]() {
        LOG_INFO("Entered Suspended state");
        transitionToState(State::Suspended);

        // Record session suspension
        if (m_sessionManager && !m_currentSessionId.isNull()) {
            QJsonObject eventData;
            eventData["state"] = "suspended";
            m_sessionManager->recordSessionEvent(m_currentSessionId, "state_change", eventData);
        }
    });

    // Reconnecting state
    connect(m_reconnectingState, &QState::entered, [this]() {
        LOG_INFO("Entered Reconnecting state");
        transitionToState(State::Reconnecting);
    });

    // Ending state
    connect(m_endingState, &QState::entered, [this]() {
        LOG_INFO("Entered Ending state");
        transitionToState(State::Ending);

        // Close the session
        if (m_sessionManager && !m_currentSessionId.isNull()) {
            m_sessionManager->closeSession(m_currentSessionId);
            emit sessionClosed(m_currentSessionId);
        }

        // Move to final state
        emit sessionEnded();
    });
}

void SessionStateMachine::setupTransitions()
{
    // Inactive to Active: When session is started
    m_inactiveState->addTransition(this, &SessionStateMachine::sessionStarted, m_activeState);

    // Active to AFK: When user goes AFK
    m_activeState->addTransition(this, &SessionStateMachine::userWentAfk, m_afkState);

    // Active to Suspended: When system suspends
    m_activeState->addTransition(this, &SessionStateMachine::systemSuspend, m_suspendedState);

    // Active to Reconnecting: When connection is lost
    m_activeState->addTransition(this, &SessionStateMachine::lostConnection, m_reconnectingState);

    // Active to Ending: When session ends
    m_activeState->addTransition(this, &SessionStateMachine::sessionEnded, m_endingState);

    // AFK to Active: When user returns
    m_afkState->addTransition(this, &SessionStateMachine::userReturnedFromAfk, m_activeState);

    // AFK to Suspended: When system suspends
    m_afkState->addTransition(this, &SessionStateMachine::systemSuspend, m_suspendedState);

    // AFK to Reconnecting: When connection is lost
    m_afkState->addTransition(this, &SessionStateMachine::lostConnection, m_reconnectingState);

    // AFK to Ending: When session ends
    m_afkState->addTransition(this, &SessionStateMachine::sessionEnded, m_endingState);

    // Suspended to Active: When system resumes (and user was active)
    m_suspendedState->addTransition(this, &SessionStateMachine::systemResume, m_activeState);

    // Suspended to Reconnecting: When connection is lost
    m_suspendedState->addTransition(this, &SessionStateMachine::lostConnection, m_reconnectingState);

    // Suspended to Ending: When session ends
    m_suspendedState->addTransition(this, &SessionStateMachine::sessionEnded, m_endingState);

    // Reconnecting to Active: When connection is restored
    m_reconnectingState->addTransition(this, &SessionStateMachine::restoredConnection, m_activeState);

    // Reconnecting to Ending: When session ends
    m_reconnectingState->addTransition(this, &SessionStateMachine::sessionEnded, m_endingState);

    // Ending to Final: Always transition to final state after ending
    m_endingState->addTransition(m_endingState, &QState::entered, m_finalState);

    // No transition from Final state - it's handled by the finished() signal
    // of the state machine which is connected to our resetStateMachine() slot
}

void SessionStateMachine::transitionToState(State newState)
{
    if (m_currentState != newState) {
        State oldState = m_currentState;
        m_currentState = newState;
        // Using static_cast to convert enum to int for signal/slot compatibility
        emit stateChanged(static_cast<int>(newState), static_cast<int>(oldState));
    }
}