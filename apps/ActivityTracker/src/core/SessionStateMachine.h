#ifndef SESSIONSTATEMACHINE_H
#define SESSIONSTATEMACHINE_H

#include <QObject>
#include <QStateMachine>
#include <QState>
#include <QFinalState>
#include <QUuid>
#include <QDateTime>

// Forward declarations
class SessionManager;

class SessionStateMachine : public QObject
{
    Q_OBJECT
public:
    enum State {
        Inactive = 0,    // No active session
        Active = 1,      // Session active, user active
        Afk = 2,         // Session active, user away
        Suspended = 3,   // Session suspended (system sleep/lock)
        Reconnecting = 4,// Attempting to reconnect to session
        Ending = 5       // Session ending
    };
    Q_ENUM(State)

    explicit SessionStateMachine(SessionManager* sessionManager, QObject *parent = nullptr);
    ~SessionStateMachine();

    bool initialize();

    State currentState() const;
    QUuid currentSessionId() const;
    QDateTime sessionStartTime() const;

public slots:
    // External state control
    void startSession(const QUuid& sessionId, const QDateTime& startTime);
    void endSession();
    void userAfk(bool isAfk);
    void systemSuspending();
    void systemResuming();
    void connectionLost();
    void connectionRestored();

private slots:
    // Handle state machine reset when final state is reached
    void resetStateMachine();

signals:
    // Internal signals for state machine
    void sessionStarted();
    void sessionEnded();
    void userWentAfk();
    void userReturnedFromAfk();
    void systemSuspend();
    void systemResume();
    void lostConnection();
    void restoredConnection();

    // External signals for observers - using int for better signal/slot compatibility
    void stateChanged(int newState, int oldState);
    void sessionClosed(const QUuid& sessionId);

private:
    QStateMachine m_stateMachine;
    QState* m_inactiveState;
    QState* m_activeState;
    QState* m_afkState;
    QState* m_suspendedState;
    QState* m_reconnectingState;
    QState* m_endingState;
    QFinalState* m_finalState;

    SessionManager* m_sessionManager;
    State m_currentState;
    QUuid m_currentSessionId;
    QDateTime m_sessionStartTime;

    void setupStates();
    void setupTransitions();
    void transitionToState(State newState);
};

#endif // SESSIONSTATEMACHINE_H