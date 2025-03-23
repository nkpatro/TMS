# API Routes Documentation

This document contains all the API routes implemented in the application, organized by controller. Each route includes its HTTP method, path, description, required inputs, and expected outputs.

## Table of Contents
1. [Authentication Routes](#authentication-routes)
2. [Session Routes](#session-routes)
3. [Activity Event Routes](#activity-event-routes)
4. [Session Event Routes](#session-event-routes)
5. [Application Routes](#application-routes)
6. [App Usage Routes](#app-usage-routes)
7. [Machine Routes](#machine-routes)
8. [System Metrics Routes](#system-metrics-routes)
9. [User Role Discipline Routes](#user-role-discipline-routes)

## Authentication Routes

| Method | Path | Description | Inputs | Outputs |
|--------|------|-------------|--------|---------|
| `POST` | `/api/auth/login` | Login | JSON body with email/username and password | JSON object with access token, refresh token, and user data |
| `POST` | `/api/auth/logout` | Logout | Authentication token in header | JSON object with success status |
| `GET` | `/api/auth/profile` | Get user profile | Authentication | JSON object with user profile data |
| `POST` | `/api/auth/refresh` | Refresh token | JSON body with refresh_token | JSON object with new access token and user data |
| `POST` | `/api/auth/service-token` | Generate service token | JSON body with username, service_id, optional computer_name, machine_id | JSON object with service token and user data |

## Session Routes

| Method | Path | Description | Inputs | Outputs |
|--------|------|-------------|--------|---------|
| `GET` | `/api/sessions` | Get all sessions | Authentication, Optional active=true parameter | JSON array of sessions |
| `GET` | `/api/sessions/<id>` | Get session by ID | Authentication, Session ID in path | JSON object of the session |
| `POST` | `/api/sessions` | Create a new session | Authentication, JSON body with username, optional machine_id, ip_address, session_data, continued_from_session | JSON object of the created session |
| `POST` | `/api/sessions/<id>/end` | End a session | Authentication, Session ID in path | JSON object of the ended session |
| `GET` | `/api/sessions/active` | Get active session for current user | Authentication, Optional machine_id parameter | JSON object of the active session |
| `GET` | `/api/users/<userId>/sessions` | Get sessions by user ID | Authentication, User ID in path, Optional active=true parameter | JSON array of sessions for the user |
| `GET` | `/api/machines/<machineId>/sessions` | Get sessions by machine ID | Authentication, Machine ID in path, Optional active=true parameter | JSON array of sessions for the machine |
| `GET` | `/api/sessions/<sessionId>/stats` | Get session statistics | Authentication, Session ID in path | JSON object with session statistics |
| `GET` | `/api/users/<userId>/stats` | Get user statistics | Authentication, User ID in path, Optional start_date and end_date parameters | JSON object with user statistics |
| `GET` | `/api/sessions/<sessionId>/chain` | Get session chain | Authentication, Session ID in path | JSON object with session chain and statistics |

### Session AFK Routes

| Method | Path | Description | Inputs | Outputs |
|--------|------|-------------|--------|---------|
| `POST` | `/api/sessions/<sessionId>/afk/start` | Start AFK period | Authentication, Session ID in path, Optional JSON body with start_time | JSON object of the created AFK period |
| `POST` | `/api/sessions/<sessionId>/afk/end` | End AFK period | Authentication, Session ID in path, Optional JSON body with end_time | JSON object of the ended AFK period |
| `GET` | `/api/sessions/<sessionId>/afk` | Get AFK periods for a session | Authentication, Session ID in path | JSON array of AFK periods for the session |

## Activity Event Routes

| Method | Path | Description | Inputs | Outputs |
|--------|------|-------------|--------|---------|
| `GET` | `/api/activities` | Get all activity events | Authentication, Optional limit parameter | JSON array of activity events |
| `GET` | `/api/activities/<id>` | Get activity event by ID | Authentication, Event ID in path | JSON object of the activity event |
| `GET` | `/api/sessions/<sessionId>/activities` | Get events by session ID | Authentication, Session ID in path, Optional limit and offset parameters | JSON array of activity events for the session |
| `GET` | `/api/sessions/<sessionId>/activities/timerange` | Get events by time range for a session | Authentication, Session ID in path, start_time and end_time query parameters, Optional limit and offset parameters | JSON array of activity events within the time range for the session |
| `POST` | `/api/activities` | Create activity event | Authentication, JSON body with session_id, optional app_id, event_type, event_time, event_data | JSON object of the created activity event |
| `POST` | `/api/sessions/<sessionId>/activities` | Create activity event for session | Authentication, Session ID in path, JSON body with optional app_id, event_type, event_time, event_data | JSON object of the created activity event |
| `PUT` | `/api/activities/<id>` | Update activity event | Authentication, Event ID in path, JSON body with fields to update (app_id, event_type, event_time, event_data) | JSON object of the updated activity event |
| `DELETE` | `/api/activities/<id>` | Delete activity event | Authentication, Event ID in path | Empty response with 204 status code |
| `GET` | `/api/sessions/<sessionId>/activities/stats` | Get activity statistics for session | Authentication, Session ID in path | JSON object with activity statistics for the session |

## Session Event Routes

| Method | Path | Description | Inputs | Outputs |
|--------|------|-------------|--------|---------|
| `GET` | `/api/session-events` | Get all session events | Authentication, Optional limit parameter | JSON array of session events |
| `GET` | `/api/session-events/<id>` | Get session event by ID | Authentication, Event ID in path | JSON object of the session event |
| `GET` | `/api/sessions/<sessionId>/events` | Get session events by session ID | Authentication, Session ID in path, Optional limit and offset parameters | JSON array of session events for the session |
| `GET` | `/api/sessions/<sessionId>/events/type/<eventType>` | Get session events by type for a session | Authentication, Session ID in path, Event type in path, Optional limit and offset parameters | JSON array of session events of the specified type for the session |
| `GET` | `/api/sessions/<sessionId>/events/timerange` | Get session events by time range for a session | Authentication, Session ID in path, start_time and end_time query parameters, Optional limit and offset parameters | JSON array of session events within the time range for the session |
| `GET` | `/api/users/<userId>/session-events` | Get session events by user ID | Authentication, User ID in path, Optional limit and offset parameters | JSON array of session events for the user |
| `GET` | `/api/machines/<machineId>/session-events` | Get session events by machine ID | Authentication, Machine ID in path, Optional limit and offset parameters | JSON array of session events for the machine |
| `POST` | `/api/session-events` | Create session event | Authentication, JSON body with session_id, event_type, event_time, user_id, previous_user_id, machine_id, terminal_session_id, is_remote, event_data | JSON object of the created session event |
| `POST` | `/api/sessions/<sessionId>/events` | Create session event for a session | Authentication, Session ID in path, JSON body with event_type, event_time, user_id, previous_user_id, machine_id, terminal_session_id, is_remote, event_data | JSON object of the created session event |
| `PUT` | `/api/session-events/<id>` | Update session event | Authentication, Event ID in path, JSON body with fields to update | JSON object of the updated session event |
| `DELETE` | `/api/session-events/<id>` | Delete session event | Authentication, Event ID in path | Empty response with 204 status code |
| `GET` | `/api/sessions/<sessionId>/events/stats` | Get session event statistics | Authentication, Session ID in path | JSON object with session event statistics |

## Application Routes

| Method | Path | Description | Inputs | Outputs |
|--------|------|-------------|--------|---------|
| `GET` | `/api/applications` | Get all applications | Authentication | JSON array of applications |
| `GET` | `/api/applications/<id>` | Get application by ID | Authentication, Application ID in path | JSON object of the application |
| `POST` | `/api/applications` | Create a new application | Authentication, JSON body with app name, app path, optional app_hash, is_restricted, tracking_enabled | JSON object of the created application |
| `PUT` | `/api/applications/<id>` | Update an application | Authentication, Application ID in path, JSON body with fields to update | JSON object of the updated application |
| `DELETE` | `/api/applications/<id>` | Delete an application | Authentication, Application ID in path | Empty response with 204 status code |
| `POST` | `/api/applications/<appId>/roles/<roleId>` | Assign application to role | Authentication, Application ID and Role ID in path | JSON object with assignment details |
| `DELETE` | `/api/applications/<appId>/roles/<roleId>` | Remove application from role | Authentication, Application ID and Role ID in path | Empty response with 204 status code |
| `POST` | `/api/applications/<appId>/disciplines/<disciplineId>` | Assign application to discipline | Authentication, Application ID and Discipline ID in path | JSON object with assignment details |
| `DELETE` | `/api/applications/<appId>/disciplines/<disciplineId>` | Remove application from discipline | Authentication, Application ID and Discipline ID in path | Empty response with 204 status code |
| `GET` | `/api/applications/restricted` | Get restricted applications | Authentication | JSON array of restricted applications |
| `GET` | `/api/applications/tracked` | Get tracked applications | Authentication | JSON array of tracked applications |
| `POST` | `/api/applications/detect` | Detect application | Authentication, JSON body with app_name, app_path, optional app_hash, is_restricted, tracking_enabled | JSON object of the detected or created application |
| `GET` | `/api/roles/<roleId>/applications` | Get applications by role | Authentication, Role ID in path | JSON array of applications assigned to the role |
| `GET` | `/api/disciplines/<disciplineId>/applications` | Get applications by discipline | Authentication, Discipline ID in path | JSON array of applications assigned to the discipline |

## App Usage Routes

| Method | Path | Description | Inputs | Outputs |
|--------|------|-------------|--------|---------|
| `GET` | `/api/app-usages` | Get all app usages | Authentication, Optional limit parameter | JSON array of app usages |
| `GET` | `/api/app-usages/<id>` | Get app usage by ID | Authentication, App usage ID in path | JSON object of the app usage |
| `GET` | `/api/sessions/<sessionId>/app-usages` | Get app usages by session ID | Authentication, Session ID in path, Optional active=true parameter | JSON array of app usages for the session |
| `GET` | `/api/applications/<appId>/usages` | Get app usages by app ID | Authentication, App ID in path, Optional limit parameter | JSON array of app usages for the application |
| `POST` | `/api/app-usages` | Start app usage | Authentication, JSON body with session_id, app_id, optional window_title, start_time | JSON object of the created app usage |
| `POST` | `/api/app-usages/<id>/end` | End app usage | Authentication, App usage ID in path, Optional JSON body with end_time | JSON object of the updated app usage |
| `PUT` | `/api/app-usages/<id>` | Update app usage | Authentication, App usage ID in path, JSON body with fields to update | JSON object of the updated app usage |
| `GET` | `/api/sessions/<sessionId>/app-usages/stats` | Get app usage stats for a session | Authentication, Session ID in path | JSON object with app usage statistics for the session |
| `GET` | `/api/sessions/<sessionId>/app-usages/top` | Get top apps for a session | Authentication, Session ID in path, Optional limit parameter | JSON object with top apps for the session |
| `GET` | `/api/sessions/<sessionId>/app-usages/active` | Get active apps for a session | Authentication, Session ID in path | JSON object with active apps for the session |

## Machine Routes

| Method | Path | Description | Inputs | Outputs |
|--------|------|-------------|--------|---------|
| `GET` | `/api/machines` | Get all machines | Authentication | JSON array of machines |
| `GET` | `/api/machines/active` | Get active machines | Authentication | JSON array of active machines |
| `GET` | `/api/machines/name/<name>` | Get machines by name | Authentication, Machine name in path | JSON array of machines with the name |
| `GET` | `/api/machines/<id>` | Get machine by ID | Authentication, Machine ID in path | JSON object of the machine |
| `POST` | `/api/machines` | Create machine | Authentication, JSON body with name, machineUniqueId or macAddress, operatingSystem, optional cpuInfo, gpuInfo, ramSizeGB, lastKnownIp | JSON object of the created machine |
| `POST` | `/api/machines/register` | Register machine | Optional Authentication, JSON body with name, operatingSystem, optional machineUniqueId, macAddress, cpuInfo, gpuInfo, ramSizeGB, lastKnownIp | JSON object of the registered machine |
| `PUT` | `/api/machines/<id>` | Update machine | Authentication, Machine ID in path, JSON body with fields to update | JSON object of the updated machine |
| `PUT` | `/api/machines/<id>/status` | Update machine status | Authentication, Machine ID in path, JSON body with active status | JSON object of the updated machine |
| `PUT` | `/api/machines/<id>/lastseen` | Update last seen timestamp | Authentication, Machine ID in path, Optional JSON body with timestamp | JSON object of the updated machine |
| `DELETE` | `/api/machines/<id>` | Delete machine | Authentication, Machine ID in path | JSON object with success status |

## System Metrics Routes

| Method | Path | Description | Inputs | Outputs |
|--------|------|-------------|--------|---------|
| `GET` | `/api/metrics` | Get all metrics | Authentication, Optional limit parameter | JSON array of system metrics |
| `GET` | `/api/sessions/<sessionId>/metrics` | Get metrics by session ID | Authentication, Session ID in path, Optional limit and offset parameters | JSON array of system metrics for the session |
| `POST` | `/api/metrics` | Record metrics | Authentication, JSON body with session_id, optional cpu_usage, gpu_usage, memory_usage, measurement_time | JSON object of the created system metrics |
| `POST` | `/api/sessions/<sessionId>/metrics` | Record metrics for a session | Authentication, Session ID in path, JSON body with optional cpu_usage, gpu_usage, memory_usage, measurement_time | JSON object of the created system metrics |
| `GET` | `/api/sessions/<sessionId>/metrics/average` | Get average metrics for a session | Authentication, Session ID in path | JSON object with average metrics for the session |
| `GET` | `/api/sessions/<sessionId>/metrics/timeseries/<metricType>` | Get metrics time series for a session | Authentication, Session ID in path, Metric type in path (cpu, gpu, memory, all) | JSON object with metrics time series for the session |
| `GET` | `/api/system/info` | Get current system information | Authentication | JSON object with current system information |

## User Role Discipline Routes

These routes handle the relationship between users, roles, and disciplines in a three-way relationship model. They allow for assigning users to specific roles within particular disciplines and checking these assignments.

| Method | Path | Description | Inputs | Outputs |
|--------|------|-------------|--------|---------|
| `GET` | `/api/user-role-disciplines` | Get all assignments | Authentication | JSON array of user-role-discipline assignments containing id, user_id, role_id, discipline_id, created_at, created_by, updated_at, updated_by |
| `GET` | `/api/users/<userId>/role-disciplines` | Get assignments for a user | Authentication, User ID in path | JSON array of role-discipline assignments for the user containing id, user_id, role_id, discipline_id, and metadata |
| `GET` | `/api/roles/<roleId>/user-disciplines` | Get assignments for a role | Authentication, Role ID in path | JSON array of user-discipline assignments for the role containing id, user_id, role_id, discipline_id, and metadata |
| `GET` | `/api/disciplines/<disciplineId>/user-roles` | Get assignments for a discipline | Authentication, Discipline ID in path | JSON array of user-role assignments for the discipline containing id, user_id, role_id, discipline_id, and metadata |
| `POST` | `/api/user-role-disciplines` | Create assignment | Authentication, JSON body with user_id (UUID), role_id (UUID), discipline_id (UUID) | JSON object of the created assignment with HTTP status 201 Created |
| `PUT` | `/api/user-role-disciplines/<id>` | Update assignment | Authentication, Assignment ID in path, JSON body with fields to update (user_id, role_id, discipline_id) | JSON object of the updated assignment |
| `DELETE` | `/api/user-role-disciplines/<id>` | Delete assignment | Authentication, Assignment ID in path | JSON object with success status {'success': true} |
| `POST` | `/api/user-role-disciplines/check` | Check if a user has a role in a discipline | Authentication, JSON body with user_id (UUID), role_id (UUID), discipline_id (UUID) | JSON object with check result {'user_id': '...', 'role_id': '...', 'discipline_id': '...', 'has_assignment': true/false} |

### Notes:
- All UUIDs are expected without braces, e.g., "550e8400-e29b-41d4-a716-446655440000"
- The system checks for existing assignments before creating new ones to avoid duplicates
- Creation timestamps and user information are automatically added to all new assignments
- The check endpoint is useful for permission validation without having to retrieve and process all assignmentstype/<eventType>` | Get events by type for a session | Authentication, Session ID in path, Event type in path, Optional limit and offset parameters | JSON array of activity events of the specified type for the session |
| `GET` | `/api/sessions/<sessionId>/activities/