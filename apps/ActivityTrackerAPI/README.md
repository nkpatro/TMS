# API Routes Documentation

This document provides a comprehensive list of all API routes organized by controller. For each route, it describes the HTTP method, path, handler function, required inputs, and expected outputs.

## Summary

Total API Routes: 89 routes across 9 controllers

| Controller | Routes |
|------------|--------|
| ActivityEventController | 10 |
| ApplicationController | 14 |
| AppUsageController | 10 | 
| AuthController | 4 |
| MachineController | 9 |
| SessionController | 15 |
| SessionEventController | 12 |
| SystemMetricsController | 7 |
| UserRoleDisciplineController | 8 |

## ActivityEventController

### 1. GET /api/activities
- **Handler**: `handleGetEvents`
- **Description**: Retrieves all activity events
- **Inputs**:
    - Authorization header (token)
    - Optional query parameters:
        - `limit`: Maximum number of events to return (default: 100)
- **Outputs**:
    - Success: JSON array of activity events
    - Error: 401 Unauthorized, 500 Internal Server Error

### 2. GET /api/activities/\<arg\>
- **Handler**: `handleGetEventById`
- **Description**: Retrieves a specific activity event by ID
- **Inputs**:
    - Authorization header (token)
    - Path parameter: event ID
- **Outputs**:
    - Success: JSON object for the specific activity event
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 3. GET /api/sessions/\<arg\>/activities
- **Handler**: `handleGetEventsBySessionId`
- **Description**: Retrieves all activity events for a specific session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - Optional query parameters:
        - `limit`: Maximum number of events to return
        - `offset`: Number of events to skip
- **Outputs**:
    - Success: JSON array of activity events for the session
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 4. GET /api/sessions/\<arg\>/activities/type/\<arg\>
- **Handler**: `handleGetEventsByEventType`
- **Description**: Retrieves activity events by type for a specific session
- **Inputs**:
    - Authorization header (token)
    - Path parameters: session ID and event type
    - Optional query parameters:
        - `limit`: Maximum number of events to return
        - `offset`: Number of events to skip
- **Outputs**:
    - Success: JSON array of filtered activity events
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 5. GET /api/sessions/\<arg\>/activities/timerange
- **Handler**: `handleGetEventsByTimeRange`
- **Description**: Retrieves activity events within a time range for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - Required query parameters:
        - `start_time`: Start time in ISO format
        - `end_time`: End time in ISO format
    - Optional query parameters:
        - `limit`: Maximum number of events to return
        - `offset`: Number of events to skip
- **Outputs**:
    - Success: JSON array of activity events within the specified time range
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 6. POST /api/activities
- **Handler**: `handleCreateEvent`
- **Description**: Creates a new activity event
- **Inputs**:
    - Authorization header (token)
    - JSON request body:
        - `session_id`: Session UUID (required)
        - `event_type`: Type of event (e.g., mouse_click, keyboard)
        - `event_time`: Time of the event (ISO format)
        - `app_id`: Application UUID (optional)
        - `event_data`: Additional event data (JSON object)
- **Outputs**:
    - Success: 201 Created with the created activity event
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 7. POST /api/sessions/\<arg\>/activities
- **Handler**: `handleCreateEventForSession`
- **Description**: Creates a new activity event for a specific session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - JSON request body:
        - `event_type`: Type of event
        - `event_time`: Time of the event (ISO format)
        - `app_id`: Application UUID (optional)
        - `event_data`: Additional event data (JSON object)
- **Outputs**:
    - Success: 201 Created with the created activity event
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 8. PUT /api/activities/\<arg\>
- **Handler**: `handleUpdateEvent`
- **Description**: Updates an existing activity event
- **Inputs**:
    - Authorization header (token)
    - Path parameter: event ID
    - JSON request body with fields to update:
        - `event_type`: Type of event
        - `event_time`: Time of the event (ISO format)
        - `app_id`: Application UUID
        - `event_data`: Additional event data (JSON object)
- **Outputs**:
    - Success: JSON object with the updated activity event
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 9. DELETE /api/activities/\<arg\>
- **Handler**: `handleDeleteEvent`
- **Description**: Deletes an activity event
- **Inputs**:
    - Authorization header (token)
    - Path parameter: event ID
- **Outputs**:
    - Success: 204 No Content
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 10. GET /api/sessions/\<arg\>/activities/stats
- **Handler**: `handleGetEventStats`
- **Description**: Retrieves activity statistics for a specific session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
- **Outputs**:
    - Success: JSON object with activity statistics
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

## ApplicationController

### 1. GET /api/applications
- **Handler**: `handleGetApplications`
- **Description**: Retrieves all applications
- **Inputs**:
    - Authorization header (token)
- **Outputs**:
    - Success: JSON array of applications
    - Error: 401 Unauthorized, 500 Internal Server Error

### 2. GET /api/applications/\<arg\>
- **Handler**: `handleGetApplicationById`
- **Description**: Retrieves a specific application by ID
- **Inputs**:
    - Authorization header (token)
    - Path parameter: application ID
- **Outputs**:
    - Success: JSON object for the specific application
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 3. POST /api/applications
- **Handler**: `handleCreateApplication`
- **Description**: Creates a new application
- **Inputs**:
    - Authorization header (token)
    - JSON request body:
        - `app_name` or `name`: Application name (required)
        - `app_path`: Application path (required)
        - `app_hash`: Application hash (optional)
        - `is_restricted`: Boolean to indicate if app is restricted
        - `tracking_enabled`: Boolean to enable tracking
- **Outputs**:
    - Success: 201 Created with the created application
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 4. PUT /api/applications/\<arg\>
- **Handler**: `handleUpdateApplication`
- **Description**: Updates an existing application
- **Inputs**:
    - Authorization header (token)
    - Path parameter: application ID
    - JSON request body with fields to update:
        - `app_name`: Application name
        - `app_path`: Application path
        - `app_hash`: Application hash
        - `is_restricted`: Boolean to indicate if app is restricted
        - `tracking_enabled`: Boolean to enable tracking
- **Outputs**:
    - Success: JSON object with the updated application
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 5. DELETE /api/applications/\<arg\>
- **Handler**: `handleDeleteApplication`
- **Description**: Deletes an application
- **Inputs**:
    - Authorization header (token)
    - Path parameter: application ID
- **Outputs**:
    - Success: 204 No Content
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 6. POST /api/applications/\<arg\>/roles/\<arg\>
- **Handler**: `handleAssignApplicationToRole`
- **Description**: Assigns an application to a role
- **Inputs**:
    - Authorization header (token)
    - Path parameters: application ID and role ID
- **Outputs**:
    - Success: JSON object with confirmation
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 7. DELETE /api/applications/\<arg\>/roles/\<arg\>
- **Handler**: `handleRemoveApplicationFromRole`
- **Description**: Removes an application from a role
- **Inputs**:
    - Authorization header (token)
    - Path parameters: application ID and role ID
- **Outputs**:
    - Success: 204 No Content
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 8. POST /api/applications/\<arg\>/disciplines/\<arg\>
- **Handler**: `handleAssignApplicationToDiscipline`
- **Description**: Assigns an application to a discipline
- **Inputs**:
    - Authorization header (token)
    - Path parameters: application ID and discipline ID
- **Outputs**:
    - Success: JSON object with confirmation
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 9. DELETE /api/applications/\<arg\>/disciplines/\<arg\>
- **Handler**: `handleRemoveApplicationFromDiscipline`
- **Description**: Removes an application from a discipline
- **Inputs**:
    - Authorization header (token)
    - Path parameters: application ID and discipline ID
- **Outputs**:
    - Success: 204 No Content
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 10. GET /api/applications/restricted
- **Handler**: `handleGetRestrictedApplications`
- **Description**: Retrieves all restricted applications
- **Inputs**:
    - Authorization header (token)
- **Outputs**:
    - Success: JSON array of restricted applications
    - Error: 401 Unauthorized, 500 Internal Server Error

### 11. GET /api/applications/tracked
- **Handler**: `handleGetTrackedApplications`
- **Description**: Retrieves all tracked applications
- **Inputs**:
    - Authorization header (token)
- **Outputs**:
    - Success: JSON array of tracked applications
    - Error: 401 Unauthorized, 500 Internal Server Error

### 12. POST /api/applications/detect
- **Handler**: `handleDetectApplication`
- **Description**: Detects or creates an application based on system info
- **Inputs**:
    - Authorization header (token)
    - JSON request body:
        - `app_name`: Application name (required)
        - `app_path`: Application path (required)
        - `app_hash`: Application hash
        - `is_restricted`: Boolean to indicate if app is restricted
        - `tracking_enabled`: Boolean to enable tracking
- **Outputs**:
    - Success: JSON object with detected/created application and newly_created flag
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 13. GET /api/roles/\<arg\>/applications
- **Handler**: `handleGetApplicationsByRole`
- **Description**: Retrieves applications associated with a role
- **Inputs**:
    - Authorization header (token)
    - Path parameter: role ID
- **Outputs**:
    - Success: JSON array of applications
    - Error: 401 Unauthorized, 500 Internal Server Error

### 14. GET /api/disciplines/\<arg\>/applications
- **Handler**: `handleGetApplicationsByDiscipline`
- **Description**: Retrieves applications associated with a discipline
- **Inputs**:
    - Authorization header (token)
    - Path parameter: discipline ID
- **Outputs**:
    - Success: JSON array of applications
    - Error: 401 Unauthorized, 500 Internal Server Error

## AppUsageController

### 1. GET /api/app-usages
- **Handler**: `handleGetAppUsages`
- **Description**: Retrieves all app usages
- **Inputs**:
    - Authorization header (token)
    - Optional query parameters:
        - `limit`: Maximum number of usages to return (default: 100)
- **Outputs**:
    - Success: JSON array of app usages
    - Error: 401 Unauthorized, 500 Internal Server Error

### 2. GET /api/app-usages/\<arg\>
- **Handler**: `handleGetAppUsageById`
- **Description**: Retrieves a specific app usage by ID
- **Inputs**:
    - Authorization header (token)
    - Path parameter: usage ID
- **Outputs**:
    - Success: JSON object for the specific app usage
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 3. GET /api/sessions/\<arg\>/app-usages
- **Handler**: `handleGetAppUsagesBySessionId`
- **Description**: Retrieves app usages for a specific session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - Optional query parameters:
        - `active`: Boolean to filter only active usages
- **Outputs**:
    - Success: JSON array of app usages
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 4. GET /api/applications/\<arg\>/usages
- **Handler**: `handleGetAppUsagesByAppId`
- **Description**: Retrieves app usages for a specific application
- **Inputs**:
    - Authorization header (token)
    - Path parameter: application ID
    - Optional query parameters:
        - `limit`: Maximum number of usages to return
- **Outputs**:
    - Success: JSON array of app usages
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 5. POST /api/app-usages
- **Handler**: `handleStartAppUsage`
- **Description**: Starts tracking app usage
- **Inputs**:
    - Authorization header (token)
    - JSON request body:
        - `session_id`: Session UUID (required)
        - `app_id`: Application UUID (required)
        - `window_title`: Current window title
        - `start_time`: Start time in ISO format
- **Outputs**:
    - Success: 201 Created with the created app usage
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 6. POST /api/app-usages/\<arg\>/end
- **Handler**: `handleEndAppUsage`
- **Description**: Ends tracking app usage
- **Inputs**:
    - Authorization header (token)
    - Path parameter: usage ID
    - Optional JSON request body:
        - `end_time`: End time in ISO format
- **Outputs**:
    - Success: JSON object with updated app usage
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 7. PUT /api/app-usages/\<arg\>
- **Handler**: `handleUpdateAppUsage`
- **Description**: Updates app usage information
- **Inputs**:
    - Authorization header (token)
    - Path parameter: usage ID
    - JSON request body with fields to update:
        - `window_title`: Window title
        - `start_time`: Start time (if active)
        - `end_time`: End time (if not active)
- **Outputs**:
    - Success: JSON object with updated app usage
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 8. GET /api/sessions/\<arg\>/app-usages/stats
- **Handler**: `handleGetSessionAppUsageStats`
- **Description**: Retrieves app usage statistics for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
- **Outputs**:
    - Success: JSON object with usage statistics
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 9. GET /api/sessions/\<arg\>/app-usages/top
- **Handler**: `handleGetTopApps`
- **Description**: Retrieves top used apps for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - Optional query parameters:
        - `limit`: Maximum number of apps to return (default: 10)
- **Outputs**:
    - Success: JSON object with top apps
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 10. GET /api/sessions/\<arg\>/app-usages/active
- **Handler**: `handleGetActiveApps`
- **Description**: Retrieves currently active apps for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
- **Outputs**:
    - Success: JSON object with active apps
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

## AuthController

### 1. POST /api/auth/login
- **Handler**: `handleLogin`
- **Description**: Authenticates a user and returns a token
- **Inputs**:
    - JSON request body:
        - `email`: User email
        - `password`: User password
        - `username`: Optional username
- **Outputs**:
    - Success: JSON object with token and user info
    - Error: 400 Bad Request, 401 Unauthorized, 403 Forbidden, 500 Internal Server Error

### 2. POST /api/auth/logout
- **Handler**: `handleLogout`
- **Description**: Logs out a user by invalidating their token
- **Inputs**:
    - Authorization header (token)
- **Outputs**:
    - Success: JSON object with success status
    - Error: 400 Bad Request, 401 Unauthorized

### 3. GET /api/auth/profile
- **Handler**: `handleGetProfile`
- **Description**: Retrieves the authenticated user's profile
- **Inputs**:
    - Authorization header (token)
- **Outputs**:
    - Success: JSON object with user profile
    - Error: 401 Unauthorized, 404 Not Found

### 4. POST /api/auth/refresh
- **Handler**: `handleRefreshToken`
- **Description**: Refreshes an authentication token
- **Inputs**:
    - Authorization header (token)
- **Outputs**:
    - Success: JSON object with new token and user info
    - Error: 401 Unauthorized

## MachineController

### 1. GET /api/machines
- **Handler**: `getAllMachines`
- **Description**: Retrieves all machines
- **Inputs**:
    - Authorization header (token)
- **Outputs**:
    - Success: JSON array of machines
    - Error: 401 Unauthorized, 500 Internal Server Error

### 2. GET /api/machines/active
- **Handler**: `getActiveMachines`
- **Description**: Retrieves all active machines
- **Inputs**:
    - Authorization header (token)
- **Outputs**:
    - Success: JSON array of active machines
    - Error: 401 Unauthorized, 500 Internal Server Error

### 3. GET /api/machines/\<arg\>
- **Handler**: `getMachineById`
- **Description**: Retrieves a specific machine by ID
- **Inputs**:
    - Path parameter: machine ID
- **Outputs**:
    - Success: JSON object for the specific machine
    - Error: 400 Bad Request, 404 Not Found, 500 Internal Server Error

### 4. POST /api/machines
- **Handler**: `createMachine`
- **Description**: Creates a new machine
- **Inputs**:
    - Authorization header (token)
    - JSON request body:
        - `name`: Machine name (required)
        - `machineUniqueId`: Unique machine identifier (required)
        - `macAddress`: MAC address (required)
        - `operatingSystem`: OS information (required)
        - `cpuInfo`: CPU information
        - `gpuInfo`: GPU information
        - `ramSizeGB`: RAM size in GB
        - `lastKnownIp`: Last known IP address
        - `userId`: User ID for association
- **Outputs**:
    - Success: 201 Created with the created machine
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 5. POST /api/machines/register
- **Handler**: `registerMachine`
- **Description**: Registers a machine in the system
- **Inputs**:
    - Authorization header (token may be service token)
    - JSON request body:
        - `name`: Machine name (required)
        - `machineUniqueId`: Unique machine identifier (required)
        - `macAddress`: MAC address (required)
        - `operatingSystem`: OS information (required)
        - `cpuInfo`: CPU information
        - `gpuInfo`: GPU information
        - `ramSizeGB`: RAM size in GB
        - `ipAddress`: Current IP address
        - `userId`: User ID for association
- **Outputs**:
    - Success: JSON object with registered machine
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 6. PUT /api/machines/\<arg\>
- **Handler**: `updateMachine`
- **Description**: Updates an existing machine
- **Inputs**:
    - Authorization header (token)
    - Path parameter: machine ID
    - JSON request body with fields to update
- **Outputs**:
    - Success: JSON object with updated machine
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 7. DELETE /api/machines/\<arg\>
- **Handler**: `deleteMachine`
- **Description**: Deletes a machine
- **Inputs**:
    - Authorization header (token)
    - Path parameter: machine ID
- **Outputs**:
    - Success: JSON object with deleted flag
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 8. PUT /api/machines/\<arg\>/status
- **Handler**: `updateMachineStatus`
- **Description**: Updates a machine's active status
- **Inputs**:
    - Authorization header (token)
    - Path parameter: machine ID
    - JSON request body:
        - `active`: Boolean status
- **Outputs**:
    - Success: JSON object with updated machine
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 9. PUT /api/machines/\<arg\>/lastseen
- **Handler**: `updateLastSeen`
- **Description**: Updates the last seen timestamp for a machine
- **Inputs**:
    - Authorization header (token)
    - Path parameter: machine ID
    - Optional JSON request body:
        - `timestamp`: Timestamp in ISO format
- **Outputs**:
    - Success: JSON object with updated machine
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

## SessionController

### 1. GET /api/sessions
- **Handler**: `handleGetSessions`
- **Description**: Retrieves all sessions
- **Inputs**:
    - Authorization header (token)
    - Optional query parameters:
        - `active`: Boolean to filter active sessions
- **Outputs**:
    - Success: JSON array of sessions
    - Error: 401 Unauthorized, 500 Internal Server Error

### 2. GET /api/sessions/\<arg\>
- **Handler**: `handleGetSessionById`
- **Description**: Retrieves a specific session by ID
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
- **Outputs**:
    - Success: JSON object for the specific session
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 3. POST /api/sessions
- **Handler**: `handleCreateSession`
- **Description**: Creates a new session
- **Inputs**:
    - Authorization header (token)
    - JSON request body:
        - `username`: Username (required)
        - `machine_id`: Machine ID
        - `login_time`: Login time in ISO format
        - `ip_address`: IP address
        - `session_data`: Additional session data (JSON object)
        - `handle_existing`: How to handle existing active sessions ("end", "continue", "use_existing")
        - `continue_from_session`: Session ID to continue from
- **Outputs**:
    - Success: 201 Created with the created session
    - Error: 400 Bad Request, 401 Unauthorized, 422 Unprocessable Entity, 500 Internal Server Error

### 4. POST /api/sessions/\<arg\>/end
- **Handler**: `handleEndSession`
- **Description**: Ends an active session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
- **Outputs**:
    - Success: JSON object with ended session
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 5. GET /api/sessions/active
- **Handler**: `handleGetActiveSession`
- **Description**: Retrieves the active session for the current user
- **Inputs**:
    - Authorization header (token)
    - Optional query parameters:
        - `machine_id`: Machine ID to filter by
- **Outputs**:
    - Success: JSON object with active session
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 6. GET /api/users/\<arg\>/sessions
- **Handler**: `handleGetSessionsByUserId`
- **Description**: Retrieves sessions for a specific user
- **Inputs**:
    - Authorization header (token)
    - Path parameter: user ID
    - Optional query parameters:
        - `active`: Boolean to filter active sessions
- **Outputs**:
    - Success: JSON array of sessions
    - Error: 401 Unauthorized, 500 Internal Server Error

### 7. GET /api/machines/\<arg\>/sessions
- **Handler**: `handleGetSessionsByMachineId`
- **Description**: Retrieves sessions for a specific machine
- **Inputs**:
    - Authorization header (token)
    - Path parameter: machine ID
    - Optional query parameters:
        - `active`: Boolean to filter active sessions
- **Outputs**:
    - Success: JSON array of sessions
    - Error: 401 Unauthorized, 500 Internal Server Error

### 8. GET /api/sessions/\<arg\>/activities
- **Handler**: `handleGetSessionActivities`
- **Description**: Retrieves activities for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - Optional query parameters:
        - `limit`: Maximum number of activities to return
        - `offset`: Number of activities to skip
- **Outputs**:
    - Success: JSON array of activities
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 9. POST /api/sessions/\<arg\>/activities
- **Handler**: `handleRecordActivity`
- **Description**: Records an activity for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - JSON request body:
        - `event_type`: Type of event
        - `event_time`: Time of the event (ISO format)
        - `app_id`: Application UUID (optional)
        - `event_data`: Additional event data (JSON object)
- **Outputs**:
    - Success: 201 Created with the recorded activity
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 10. POST /api/sessions/\<arg\>/afk/start
- **Handler**: `handleStartAfk`
- **Description**: Starts an AFK (Away From Keyboard) period
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - Optional JSON request body:
        - `start_time`: Start time in ISO format
- **Outputs**:
    - Success: 201 Created with the created AFK period
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 11. POST /api/sessions/\<arg\>/afk/end
- **Handler**: `handleEndAfk`
- **Description**: Ends an active AFK period
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - Optional JSON request body:
        - `end_time`: End time in ISO format
- **Outputs**:
    - Success: JSON object with ended AFK period
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 12. GET /api/sessions/\<arg\>/afk
- **Handler**: `handleGetAfkPeriods`
- **Description**: Retrieves AFK periods for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
- **Outputs**:
    - Success: JSON array of AFK periods
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 13. GET /api/sessions/\<arg\>/stats
- **Handler**: `handleGetSessionStats`
- **Description**: Retrieves statistics for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
- **Outputs**:
    - Success: JSON object with session statistics
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 14. GET /api/users/\<arg\>/stats
- **Handler**: `handleGetUserStats`
- **Description**: Retrieves session statistics for a user
- **Inputs**:
    - Authorization header (token)
    - Path parameter: user ID
    - Optional query parameters:
        - `start_date`: Start date for statistics (ISO format)
        - `end_date`: End date for statistics (ISO format)
- **Outputs**:
    - Success: JSON object with user statistics
    - Error: 401 Unauthorized, 500 Internal Server Error

### 15. GET /api/sessions/\<arg\>/chain
- **Handler**: `handleGetSessionChain`
- **Description**: Retrieves connected sessions chain
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
- **Outputs**:
    - Success: JSON object with session chain and statistics
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

## SessionEventController

### 1. GET /api/session-events
- **Handler**: `handleGetEvents`
- **Description**: Retrieves all session events
- **Inputs**:
    - Authorization header (token)
    - Optional query parameters:
        - `limit`: Maximum number of events to return (default: 100)
- **Outputs**:
    - Success: JSON array of session events
    - Error: 401 Unauthorized, 500 Internal Server Error

### 2. GET /api/session-events/\<arg\>
- **Handler**: `handleGetEventById`
- **Description**: Retrieves a specific session event by ID
- **Inputs**:
    - Authorization header (token)
    - Path parameter: event ID
- **Outputs**:
    - Success: JSON object for the specific session event
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 3. GET /api/sessions/\<arg\>/events
- **Handler**: `handleGetEventsBySessionId`
- **Description**: Retrieves session events for a specific session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - Optional query parameters:
        - `limit`: Maximum number of events to return
        - `offset`: Number of events to skip
- **Outputs**:
    - Success: JSON array of session events
    - Error: 401 Unauthorized, 500 Internal Server Error

### 4. GET /api/sessions/\<arg\>/events/type/\<arg\>
- **Handler**: `handleGetEventsByEventType`
- **Description**: Retrieves session events by type for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameters: session ID and event type
    - Optional query parameters:
        - `limit`: Maximum number of events to return
        - `offset`: Number of events to skip
- **Outputs**:
    - Success: JSON array of session events
    - Error: 401 Unauthorized, 500 Internal Server Error

### 5. GET /api/sessions/\<arg\>/events/timerange
- **Handler**: `handleGetEventsByTimeRange`
- **Description**: Retrieves session events within a time range
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - Required query parameters:
        - `start_time`: Start time in ISO format
        - `end_time`: End time in ISO format
    - Optional query parameters:
        - `limit`: Maximum number of events to return
        - `offset`: Number of events to skip
- **Outputs**:
    - Success: JSON array of session events
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 6. GET /api/users/\<arg\>/session-events
- **Handler**: `handleGetEventsByUserId`
- **Description**: Retrieves session events for a specific user
- **Inputs**:
    - Authorization header (token)
    - Path parameter: user ID
    - Optional query parameters:
        - `limit`: Maximum number of events to return
        - `offset`: Number of events to skip
- **Outputs**:
    - Success: JSON array of session events
    - Error: 401 Unauthorized, 500 Internal Server Error

### 7. GET /api/machines/\<arg\>/session-events
- **Handler**: `handleGetEventsByMachineId`
- **Description**: Retrieves session events for a specific machine
- **Inputs**:
    - Authorization header (token)
    - Path parameter: machine ID
    - Optional query parameters:
        - `limit`: Maximum number of events to return
        - `offset`: Number of events to skip
- **Outputs**:
    - Success: JSON array of session events
    - Error: 401 Unauthorized, 500 Internal Server Error

### 8. POST /api/session-events
- **Handler**: `handleCreateEvent`
- **Description**: Creates a new session event
- **Inputs**:
    - Authorization header (token)
    - JSON request body:
        - `session_id`: Session UUID (required)
        - `event_type`: Type of event (login, logout, lock, unlock, etc.)
        - `event_time`: Time of the event (ISO format)
        - `user_id`: User UUID
        - `previous_user_id`: Previous user UUID (for switch_user events)
        - `machine_id`: Machine UUID
        - `terminal_session_id`: Terminal session ID
        - `is_remote`: Boolean indicating if session is remote
        - `event_data`: Additional event data (JSON object)
- **Outputs**:
    - Success: 201 Created with the created event
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 9. POST /api/sessions/\<arg\>/events
- **Handler**: `handleCreateEventForSession`
- **Description**: Creates a new session event for a specific session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - JSON request body:
        - `event_type`: Type of event
        - `event_time`: Time of the event (ISO format)
        - Additional event properties (similar to handleCreateEvent)
- **Outputs**:
    - Success: 201 Created with the created event
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 10. PUT /api/session-events/\<arg\>
- **Handler**: `handleUpdateEvent`
- **Description**: Updates an existing session event
- **Inputs**:
    - Authorization header (token)
    - Path parameter: event ID
    - JSON request body with fields to update
- **Outputs**:
    - Success: JSON object with the updated event
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 11. DELETE /api/session-events/\<arg\>
- **Handler**: `handleDeleteEvent`
- **Description**: Deletes a session event
- **Inputs**:
    - Authorization header (token)
    - Path parameter: event ID
- **Outputs**:
    - Success: 204 No Content
    - Error: 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 12. GET /api/sessions/\<arg\>/events/stats
- **Handler**: `handleGetEventStats`
- **Description**: Retrieves session event statistics
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
- **Outputs**:
    - Success: JSON object with event statistics
    - Error: 401 Unauthorized, 500 Internal Server Error

## SystemMetricsController

### 1. GET /api/metrics
- **Handler**: `handleGetMetrics`
- **Description**: Retrieves system metrics
- **Inputs**:
    - Authorization header (token)
    - Optional query parameters:
        - `limit`: Maximum number of metrics to return (default: 100)
- **Outputs**:
    - Success: JSON array of metrics
    - Error: 401 Unauthorized, 500 Internal Server Error

### 2. GET /api/sessions/\<arg\>/metrics
- **Handler**: `handleGetMetricsBySessionId`
- **Description**: Retrieves metrics for a specific session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - Optional query parameters:
        - `limit`: Maximum number of metrics to return
        - `offset`: Number of metrics to skip
- **Outputs**:
    - Success: JSON array of metrics
    - Error: 401 Unauthorized, 500 Internal Server Error

### 3. POST /api/metrics
- **Handler**: `handleRecordMetrics`
- **Description**: Records system metrics
- **Inputs**:
    - Authorization header (token)
    - JSON request body:
        - `session_id`: Session UUID (required)
        - `cpu_usage`: CPU usage percentage
        - `gpu_usage`: GPU usage percentage
        - `memory_usage`: Memory usage percentage
        - `measurement_time`: Measurement time in ISO format
- **Outputs**:
    - Success: 201 Created with the recorded metrics
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 4. POST /api/sessions/\<arg\>/metrics
- **Handler**: `handleRecordMetricsForSession`
- **Description**: Records system metrics for a specific session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
    - JSON request body:
        - `cpu_usage`: CPU usage percentage
        - `gpu_usage`: GPU usage percentage
        - `memory_usage`: Memory usage percentage
        - `measurement_time`: Measurement time in ISO format
- **Outputs**:
    - Success: 201 Created with the recorded metrics
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 5. GET /api/sessions/\<arg\>/metrics/average
- **Handler**: `handleGetAverageMetrics`
- **Description**: Retrieves average metrics for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameter: session ID
- **Outputs**:
    - Success: JSON object with average metrics
    - Error: 401 Unauthorized, 500 Internal Server Error

### 6. GET /api/sessions/\<arg\>/metrics/timeseries/\<arg\>
- **Handler**: `handleGetMetricsTimeSeries`
- **Description**: Retrieves metrics time series for a session
- **Inputs**:
    - Authorization header (token)
    - Path parameters: session ID and metric type (cpu, gpu, memory, or all)
- **Outputs**:
    - Success: JSON object with time series data
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error

### 7. GET /api/system/info
- **Handler**: `handleGetSystemInfo`
- **Description**: Retrieves current system information
- **Inputs**:
    - Authorization header (token)
- **Outputs**:
    - Success: JSON object with system information
    - Error: 401 Unauthorized, 500 Internal Server Error

## UserRoleDisciplineController

### 1. GET /api/user-role-disciplines
- **Handler**: `handleGetAllAssignments`
- **Description**: Retrieves all user-role-discipline assignments
- **Inputs**:
    - Authorization header (token)
- **Outputs**:
    - Success: JSON array of assignments
    - Error: 401 Unauthorized, 500 Internal Server Error

### 2. GET /api/users/\<arg\>/role-disciplines
- **Handler**: `handleGetUserAssignments`
- **Description**: Retrieves role-discipline assignments for a user
- **Inputs**:
    - Authorization header (token)
    - Path parameter: user ID
- **Outputs**:
    - Success: JSON array of assignments
    - Error: 401 Unauthorized, 500 Internal Server Error

### 3. GET /api/roles/\<arg\>/user-disciplines
- **Handler**: `handleGetRoleAssignments`
- **Description**: Retrieves user-discipline assignments for a role
- **Inputs**:
    - Authorization header (token)
    - Path parameter: role ID
- **Outputs**:
    - Success: JSON array of assignments
    - Error: 401 Unauthorized, 500 Internal Server Error

### 4. GET /api/disciplines/\<arg\>/user-roles
- **Handler**: `handleGetDisciplineAssignments`
- **Description**: Retrieves user-role assignments for a discipline
- **Inputs**:
    - Authorization header (token)
    - Path parameter: discipline ID
- **Outputs**:
    - Success: JSON array of assignments
    - Error: 401 Unauthorized, 500 Internal Server Error

### 5. POST /api/user-role-disciplines
- **Handler**: `handleAssignRoleDiscipline`
- **Description**: Creates a new user-role-discipline assignment
- **Inputs**:
    - Authorization header (token)
    - JSON request body:
        - `user_id`: User UUID (required)
        - `role_id`: Role UUID (required)
        - `discipline_id`: Discipline UUID (required)
- **Outputs**:
    - Success: 201 Created with the created assignment
    - Error: 400 Bad Request, 401 Unauthorized, 409 Conflict, 500 Internal Server Error

### 6. PUT /api/user-role-disciplines/\<arg\>
- **Handler**: `handleUpdateAssignment`
- **Description**: Updates an existing assignment
- **Inputs**:
    - Authorization header (token)
    - Path parameter: assignment ID
    - JSON request body with fields to update:
        - `user_id`: User UUID
        - `role_id`: Role UUID
        - `discipline_id`: Discipline UUID
- **Outputs**:
    - Success: JSON object with the updated assignment
    - Error: 400 Bad Request, 401 Unauthorized, 404 Not Found, 500 Internal Server Error

### 7. DELETE /api/user-role-disciplines/\<arg\>
- **Handler**: `handleRemoveAssignment`
- **Description**: Removes an assignment
- **Inputs**:
    - Authorization header (token)
    - Path parameter: assignment ID
- **Outputs**:
    - Success: JSON object with success flag
    - Error: 401 Unauthorized, 500 Internal Server Error

### 8. POST /api/user-role-disciplines/check
- **Handler**: `handleCheckAssignment`
- **Description**: Checks if a user has a role in a discipline
- **Inputs**:
    - Authorization header (token)
    - JSON request body:
        - `user_id`: User UUID (required)
        - `role_id`: Role UUID (required)
        - `discipline_id`: Discipline UUID (required)
- **Outputs**:
    - Success: JSON object with check result
    - Error: 400 Bad Request, 401 Unauthorized, 500 Internal Server Error