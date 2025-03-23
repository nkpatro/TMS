# Activity Tracker Integration Guide

This guide explains how the refactored components fit together and how to integrate them into your existing codebase.

## Architectural Overview

The refactored Activity Tracker uses a modular, event-driven architecture with clear separation of concerns:

```
┌───────────────────┐     ┌───────────────────┐     ┌─────────────────┐
│ ActivityTracker   │     │ MonitorManager    │     │ Platform-Specific│
│ Service           │◄───►│                   │◄───►│ Monitors         │
└───────┬───────────┘     └───────────────────┘     └─────────────────┘
        │
        │
┌───────▼───────────┐     ┌───────────────────┐     ┌─────────────────┐
│ ActivityTracker   │     │ ActivityMonitor   │     │ SessionState    │
│ Client            │◄───►│ Batcher           │     │ Machine         │
└───────┬───────────┘     └───────────────────┘     └─────────────────┘
        │
        │
┌───────▼───────────┐     ┌───────────────────┐     ┌─────────────────┐
│ SyncManager       │◄───►│ SessionManager    │     │ ConfigManager   │
└───────┬───────────┘     └───────────────────┘     └─────────────────┘
        │
        │
┌───────▼───────────┐
│ APIManager        │
└───────────────────┘
```

## Key Components and Responsibilities

1. **ActivityTrackerService**
  - Service entry point
  - Manages configuration and user sessions
  - Creates and controls ActivityTrackerClient

2. **ActivityTrackerClient**
  - Core component that orchestrates all other modules
  - Manages session lifecycle and data flow
  - Connects monitor events with data synchronization

3. **MonitorManager**
  - Creates and manages platform-specific monitors
  - Provides unified interface to all monitoring functionality
  - Controls platform-specific implementations

4. **ActivityMonitorBatcher**
  - Aggregates high-frequency input events
  - Reduces API calls and improves performance
  - Provides meaningful activity summaries

5. **SessionStateMachine**
  - Manages explicit session states and transitions
  - Ensures consistent handling of state changes
  - Simplifies complex state management logic

6. **SyncManager**
  - Handles online/offline transitions
  - Queues and batches data for efficient syncing
  - Implements retry logic for failed requests

7. **ConfigManager**
  - Centralizes configuration management
  - Handles config file I/O and validation
  - Updates component settings when config changes

## Integration Steps

1. **Update Project Structure**

   Create the following folder structure:
   ```
   src/
   ├── core/
   │   └── (Core components)
   ├── managers/
   │   └── (Manager components)
   ├── monitors/
   │   ├── win/
   │   ├── mac/
   │   └── linux/
   └── service/
       └── (Service components)
   ```

2. **Update CMakeLists.txt**

   Modify your main CMakeLists.txt to include the new components:

   ```cmake
   # Core functionality
   set(CORE_SOURCES
       src/core/ActivityTrackerClient.cpp
       src/core/ActivityMonitorBatcher.cpp
       src/core/APIManager.cpp
       src/core/SessionManager.cpp
       src/core/SessionStateMachine.cpp
       src/core/SyncManager.cpp
   )

   # Manager components
   set(MANAGER_SOURCES
       src/managers/ConfigManager.cpp
       src/managers/MonitorManager.cpp
   )

   # Add to appropriate targets
   ```

3. **Integrate Components Incrementally**

   a. First replace ServiceConfig with ConfigManager
   b. Then integrate MonitorManager to replace direct monitor access
   c. Update the client with more modular components
   d. Finally update the service class

4. **API Compatibility**

   The refactored ActivityTrackerClient maintains the same public API for backward compatibility:
  - initialize()
  - start()
  - stop()
  - isRunning()
  - setDataSendInterval()
  - setIdleTimeThreshold()

## Implementation Notes

### Session Management Flow

```
1. Service starts
2. ConfigManager loads and fetches config
3. ActivityTrackerClient initializes
4. SyncManager creates or reopens a session
5. SessionStateMachine transitions to Active state
6. MonitorManager starts all monitors
7. ActivityMonitorBatcher processes input events
8. SyncManager periodically syncs data to server
```

### Day Change Handling

```
1. Day change timer triggers checkDayChange()
2. ActivityTrackerClient detects date change
3. SessionStateMachine transitions to Ending state
4. Previous session is closed on server
5. New session is created for the current day
6. SessionStateMachine transitions to Active state
```

### Error and Connection Handling

```
1. SyncManager periodically checks server connectivity
2. On connection loss, enters offline mode
3. Data continues to be collected and queued
4. On connection restoration, queued data is synced
5. SessionStateMachine handles reconnection logic
```

## Testing Recommendations

1. Test each component independently with unit tests
2. Test the connection/disconnection handling
3. Test day changeover logic
4. Verify proper cleanup on shutdown
5. Test multi-user scenarios if enabled

## Migration Timeline

1. Start with implementing the core components
2. Add managers and integration layer
3. Update service layer and command-line interface
4. Thorough testing in all supported environments