#ifndef EVENTTYPES_H
#define EVENTTYPES_H

#include <QObject>

namespace EventTypes {

    // Corresponds to the event_type ENUM in the database
    Q_NAMESPACE
    enum class ActivityEventType {
        MouseClick,
        MouseMove,
        Keyboard,
        AfkStart,
        AfkEnd,
        AppFocus,
        AppUnfocus
    };
    Q_ENUM_NS(ActivityEventType)

    // Corresponds to the session_event_type ENUM in the database
    Q_NAMESPACE
    enum class SessionEventType {
        Login,
        Logout,
        Lock,
        Unlock,
        SwitchUser,
        RemoteConnect,
        RemoteDisconnect
    };
    Q_ENUM_NS(SessionEventType)

}

#endif // EVENTTYPES_H