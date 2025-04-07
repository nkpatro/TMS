-- Drop and recreate the database
-- Note: This should be run as a superuser or database owner
-- If you're using a tool like pgAdmin, you might need to disconnect all users first
-- DROP DATABASE IF EXISTS activity_tracker;
-- CREATE DATABASE activity_tracker;

-- Connect to the database
-- \c activity_tracker

-- Create extension if not exists
CREATE EXTENSION IF NOT EXISTS "btree_gist";

-- Create archive schema if not exists
CREATE SCHEMA IF NOT EXISTS archive;

-- Custom immutable function for date ranges
CREATE OR REPLACE FUNCTION immutable_daterange(
    start_time TIMESTAMP,
    end_time TIMESTAMP
) RETURNS daterange
AS $$
BEGIN
RETURN daterange(start_time::date, COALESCE(end_time::date, 'infinity'::date));
END;
$$ LANGUAGE plpgsql IMMUTABLE;

-- Check if event_type already exists before creating
DO
$$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'event_type') THEN
CREATE TYPE event_type AS ENUM (
            'mouse_click',
            'mouse_move',
            'keyboard',
            'afk_start',
            'afk_end',
            'app_focus',
            'app_unfocus'
        );
END IF;
END
$$;

-- Check if session_event_type already exists before creating
DO
$$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'session_event_type') THEN
CREATE TYPE session_event_type AS ENUM (
            'login',
            'logout',
            'lock',
            'unlock',
            'switch_user',
            'remote_connect',
            'remote_disconnect'
        );
END IF;
END
$$;

-- Create users table if not exists - aligned with UserModel
CREATE TABLE IF NOT EXISTS users (
                                     id uuid DEFAULT gen_random_uuid() PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    photo VARCHAR(512),
    active BOOLEAN DEFAULT true NOT NULL,
    verified BOOLEAN DEFAULT false NOT NULL,
    verification_code VARCHAR(100),
    status_id uuid,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    created_by uuid,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    updated_by uuid
    );

-- Only add constraints if they don't exist
DO
$$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM pg_constraint WHERE conname = 'users_created_by_fkey'
    ) THEN
ALTER TABLE users
    ADD CONSTRAINT users_created_by_fkey
        FOREIGN KEY (created_by) REFERENCES users(id);
END IF;

    IF NOT EXISTS (
        SELECT 1 FROM pg_constraint WHERE conname = 'users_updated_by_fkey'
    ) THEN
ALTER TABLE users
    ADD CONSTRAINT users_updated_by_fkey
        FOREIGN KEY (updated_by) REFERENCES users(id);
END IF;
END
$$;

-- Create roles table if not exists - aligned with RoleModel
CREATE TABLE IF NOT EXISTS roles (
                                     id uuid DEFAULT gen_random_uuid() PRIMARY KEY,
    code VARCHAR(50) UNIQUE NOT NULL,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    created_by uuid REFERENCES users(id),
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id)
    );

-- Create disciplines table if not exists - aligned with DisciplineModel
CREATE TABLE IF NOT EXISTS disciplines (
                                           id uuid DEFAULT gen_random_uuid() PRIMARY KEY,
    code VARCHAR(50) UNIQUE NOT NULL,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    created_by uuid REFERENCES users(id),
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id)
    );

-- Create applications table if not exists
CREATE TABLE IF NOT EXISTS applications (
                                            id uuid DEFAULT gen_random_uuid() PRIMARY KEY,
    app_name VARCHAR(255) NOT NULL,
    app_path VARCHAR(512),
    app_hash VARCHAR(64),
    is_restricted BOOLEAN DEFAULT false NOT NULL,
    tracking_enabled BOOLEAN DEFAULT true NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    created_by uuid REFERENCES users(id),
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id),
    UNIQUE(app_name, app_path)
    );

-- Create junction tables if not exist
CREATE TABLE IF NOT EXISTS tracked_applications_roles (
                                                          app_id uuid REFERENCES applications(id),
    role_id uuid REFERENCES roles(id),
    created_by uuid REFERENCES users(id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id),
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    PRIMARY KEY (app_id, role_id)
    );

CREATE TABLE IF NOT EXISTS tracked_applications_disciplines (
                                                                app_id uuid REFERENCES applications(id),
    discipline_id uuid REFERENCES disciplines(id),
    created_by uuid REFERENCES users(id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id),
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    PRIMARY KEY (app_id, discipline_id)
    );

-- Create machines table
CREATE TABLE IF NOT EXISTS machines (
                                        id uuid DEFAULT gen_random_uuid() PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    machine_unique_id VARCHAR(255) NOT NULL,
    mac_address VARCHAR(100),
    operating_system VARCHAR(255),
    cpu_info VARCHAR(512),
    gpu_info VARCHAR(512),
    ram_size_gb INTEGER,
    ip_address INET,
    last_seen_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    active BOOLEAN DEFAULT true NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    created_by uuid REFERENCES users(id),
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id),
    CONSTRAINT machine_unique_id_unique UNIQUE (machine_unique_id)
    );

-- Create user_role_disciplines table to manage the relationship separately
CREATE TABLE IF NOT EXISTS user_role_disciplines (
                                                     id uuid DEFAULT gen_random_uuid() PRIMARY KEY,
    user_id uuid REFERENCES users(id) NOT NULL,
    role_id uuid REFERENCES roles(id) NOT NULL,
    discipline_id uuid REFERENCES disciplines(id) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    created_by uuid REFERENCES users(id),
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id),
    -- Enforce uniqueness of user-role-discipline combination
    UNIQUE(user_id, role_id, discipline_id)
    );

-- Create sessions table if not exists
CREATE TABLE IF NOT EXISTS sessions (
                                        id uuid DEFAULT gen_random_uuid() PRIMARY KEY,
    user_id uuid REFERENCES users(id) NOT NULL,
    machine_id uuid REFERENCES machines(id),
    login_time TIMESTAMP NOT NULL,
    logout_time TIMESTAMP,
    session_data JSONB DEFAULT '{}',
    continued_from_session UUID,
    continued_by_session UUID,
    previous_session_end_time TIMESTAMP,
    time_since_previous_session BIGINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    created_by uuid REFERENCES users(id),
    updated_at TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id),
    CONSTRAINT session_time_check CHECK (logout_time IS NULL OR logout_time > login_time),
    CONSTRAINT no_overlapping_sessions EXCLUDE USING gist (
                                                              user_id WITH =,
                                                              machine_id WITH =,
                                                              immutable_daterange(login_time, logout_time) WITH &&
                                                   )
    );

-- Add self-references after table creation
ALTER TABLE sessions
    ADD CONSTRAINT fk_continued_from_session FOREIGN KEY (continued_from_session)
        REFERENCES sessions(id) ON DELETE SET NULL;

ALTER TABLE sessions
    ADD CONSTRAINT fk_continued_by_session FOREIGN KEY (continued_by_session)
        REFERENCES sessions(id) ON DELETE SET NULL;

-- Create afk_periods table if not exists
CREATE TABLE IF NOT EXISTS afk_periods (
                                           id uuid DEFAULT gen_random_uuid() PRIMARY KEY,
    session_id uuid REFERENCES sessions(id),
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    created_by uuid REFERENCES users(id),
    updated_at TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id),
    CONSTRAINT afk_time_check CHECK (end_time IS NULL OR end_time > start_time)
    );

-- Create partitioned tables if they don't exist
DO
$$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_tables WHERE tablename = 'activity_events') THEN
CREATE TABLE activity_events (
                                 id uuid DEFAULT gen_random_uuid(),
                                 session_id uuid REFERENCES sessions(id),
                                 app_id uuid REFERENCES applications(id),
                                 event_type event_type NOT NULL,
                                 event_time TIMESTAMP NOT NULL,
                                 event_data JSONB DEFAULT '{}',
                                 created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
                                 created_by uuid REFERENCES users(id),
                                 updated_at TIMESTAMP NOT NULL,
                                 updated_by uuid REFERENCES users(id)
) PARTITION BY RANGE (event_time);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_tables WHERE tablename = 'system_metrics') THEN
CREATE TABLE system_metrics (
                                id uuid DEFAULT gen_random_uuid(),
                                session_id uuid REFERENCES sessions(id),
                                cpu_usage NUMERIC(5,2) CHECK (cpu_usage >= 0 AND cpu_usage <= 100),
                                gpu_usage NUMERIC(5,2) CHECK (gpu_usage >= 0 AND gpu_usage <= 100),
                                memory_usage NUMERIC(5,2) CHECK (memory_usage >= 0 AND memory_usage <= 100),
                                measurement_time TIMESTAMP NOT NULL,
                                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
                                created_by uuid REFERENCES users(id),
                                updated_at TIMESTAMP NOT NULL,
                                updated_by uuid REFERENCES users(id)
) PARTITION BY RANGE (measurement_time);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_tables WHERE tablename = 'app_usage') THEN
CREATE TABLE app_usage (
                           id uuid DEFAULT gen_random_uuid(),
                           session_id uuid REFERENCES sessions(id),
                           app_id uuid REFERENCES applications(id),
                           start_time TIMESTAMP NOT NULL,
                           end_time TIMESTAMP,
                           is_active BOOLEAN DEFAULT true,
                           window_title TEXT,
                           created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
                           created_by uuid REFERENCES users(id),
                           updated_at TIMESTAMP NOT NULL,
                           updated_by uuid REFERENCES users(id),
                           CONSTRAINT app_usage_time_check CHECK (end_time IS NULL OR end_time > start_time)
) PARTITION BY RANGE (start_time);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_tables WHERE tablename = 'session_events') THEN
CREATE TABLE session_events (
                                id uuid DEFAULT gen_random_uuid(),
                                session_id uuid REFERENCES sessions(id),
                                event_type session_event_type NOT NULL,
                                event_time TIMESTAMP NOT NULL,
                                user_id uuid REFERENCES users(id),
                                previous_user_id uuid REFERENCES users(id),
                                machine_id uuid REFERENCES machines(id),
                                terminal_session_id VARCHAR(50),
                                is_remote BOOLEAN DEFAULT false,
                                event_data JSONB DEFAULT '{}',
                                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
                                created_by uuid REFERENCES users(id),
                                updated_at TIMESTAMP NOT NULL,
                                updated_by uuid REFERENCES users(id)
) PARTITION BY RANGE (event_time);
END IF;
END
$$;

-- Create maintenance_history table if not exists
CREATE TABLE IF NOT EXISTS maintenance_history (
                                                   id uuid DEFAULT gen_random_uuid() PRIMARY KEY,
    task_name text NOT NULL,
    start_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    end_time TIMESTAMP,
    status text NOT NULL,
    error_message text,
    affected_partitions text[],
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    CONSTRAINT unique_task_execution UNIQUE (task_name, start_time)
    );

-- Create or replace functions for partition management
CREATE OR REPLACE FUNCTION create_partition_for_month(
    table_name text,
    partition_date date
) RETURNS void AS
$BODY$
DECLARE
partition_name text;
    start_date date;
    end_date date;
sql text;
BEGIN
    partition_name := table_name || '_y' ||
                     to_char(partition_date, 'YYYY') ||
                     'm' ||
                     to_char(partition_date, 'MM');

    start_date := date_trunc('month', partition_date);
    end_date := date_trunc('month', partition_date + interval '1 month');

sql := format(
        'CREATE TABLE IF NOT EXISTS %I PARTITION OF %I FOR VALUES FROM (%L) TO (%L)',
        partition_name,
        table_name,
        start_date,
        end_date
    );

EXECUTE sql;

EXECUTE format(
        'CREATE INDEX IF NOT EXISTS %I ON %I (created_at)',
        partition_name || '_created_at_idx',
        partition_name
        );

IF table_name = 'activity_events' THEN
        EXECUTE format(
            'CREATE INDEX IF NOT EXISTS %I ON %I (session_id, event_type)',
            partition_name || '_session_event_idx',
            partition_name
        );
    ELSIF table_name = 'system_metrics' THEN
        EXECUTE format(
            'CREATE INDEX IF NOT EXISTS %I ON %I (session_id)',
            partition_name || '_session_idx',
            partition_name
        );
END IF;
END;
$BODY$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION create_future_partitions() RETURNS void AS
$BODY$
DECLARE
tables text[] := ARRAY['activity_events', 'system_metrics', 'app_usage', 'session_events'];
    table_name text;
    future_date date;
BEGIN
    FOREACH table_name IN ARRAY tables LOOP
        FOR i IN 1..3 LOOP
            future_date := date_trunc('month', CURRENT_DATE + (i || ' month')::interval);
            PERFORM create_partition_for_month(table_name, future_date);
END LOOP;
END LOOP;
END;
$BODY$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION detach_old_partitions(
    months_to_keep int DEFAULT 3
) RETURNS void AS
$BODY$
DECLARE
cutoff_date date;
    tables text[] := ARRAY['activity_events', 'system_metrics', 'app_usage', 'session_events'];
    table_name text;
    partition_name text;
sql text;
BEGIN
    cutoff_date := date_trunc('month', CURRENT_DATE - (months_to_keep || ' month')::interval);

    FOREACH table_name IN ARRAY tables LOOP
        FOR partition_name IN
SELECT partitiontablename::text
FROM pg_partitions
WHERE tablename = table_name
  AND to_date(split_part(partitiontablename, '_y', 2), 'YYYYmMM') < cutoff_date
    LOOP
            sql := format(
                'ALTER TABLE %I DETACH PARTITION %I',
                table_name,
                partition_name
            );
EXECUTE sql;

sql := format(
                'ALTER TABLE %I SET SCHEMA archive',
                partition_name
            );
EXECUTE sql;
END LOOP;
END LOOP;
END;
$BODY$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION maintain_partitions() RETURNS void AS
$BODY$
BEGIN
    PERFORM create_future_partitions();
    PERFORM detach_old_partitions(3);
END;
$BODY$
LANGUAGE plpgsql;

-- Create or replace function to get the full chain for a session
CREATE OR REPLACE FUNCTION get_session_chain(p_session_id UUID)
RETURNS TABLE (
    chain_id UUID,
    chain_position INT,
    session_id UUID,
    user_id UUID,
    machine_id UUID,
    login_time TIMESTAMP,
    logout_time TIMESTAMP,
    gap_seconds NUMERIC,
    is_current BOOLEAN
) AS
$BODY$
DECLARE
root_id UUID;
BEGIN
    -- Find the root session for this chain
WITH RECURSIVE session_path AS (
    -- Start with the provided session
    SELECT
        id,
        continued_from_session,
        1 as depth
    FROM sessions
    WHERE id = p_session_id

    UNION ALL

    -- Go up the chain to find the root
    SELECT
        s.id,
        s.continued_from_session,
        sp.depth + 1
    FROM sessions s
             JOIN session_path sp ON s.id = sp.continued_from_session
    WHERE s.continued_from_session IS NOT NULL
)
SELECT id INTO root_id
FROM session_path
ORDER BY depth DESC
    LIMIT 1;

-- Return the chain data
RETURN QUERY
    WITH RECURSIVE forwards_chain AS (
            -- Start with the root session
            SELECT
                id,
                user_id,
                machine_id,
                login_time,
                logout_time,
                continued_by_session,
                1 as chain_position,
                id as chain_id
            FROM sessions
            WHERE id = root_id

            UNION ALL

            -- Follow the chain forwards
            SELECT
                s.id,
                s.user_id,
                s.machine_id,
                s.login_time,
                s.logout_time,
                s.continued_by_session,
                fc.chain_position + 1,
                fc.chain_id
            FROM sessions s
            JOIN forwards_chain fc ON s.continued_from_session = fc.id
        )
SELECT
    chain_id,
    chain_position,
    id as session_id,
    user_id,
    machine_id,
    login_time,
    logout_time,
    EXTRACT(EPOCH FROM (login_time - LAG(logout_time) OVER (ORDER BY chain_position))) as gap_seconds,
    CASE WHEN logout_time IS NULL THEN TRUE ELSE FALSE END as is_current
FROM forwards_chain
ORDER BY chain_position;
END;
$BODY$
LANGUAGE plpgsql;

-- Create a function to calculate session chain statistics
CREATE OR REPLACE FUNCTION get_session_chain_stats(p_session_id UUID)
RETURNS TABLE (
    chain_id UUID,
    total_sessions INT,
    first_login TIMESTAMP,
    last_activity TIMESTAMP,
    total_duration_seconds NUMERIC,
    total_gap_seconds NUMERIC,
    real_time_span_seconds NUMERIC,
    continuity_percentage NUMERIC
) AS
$BODY$
BEGIN
RETURN QUERY
    WITH chain_data AS (
            SELECT * FROM get_session_chain(p_session_id)
        )
SELECT
    MIN(chain_id) as chain_id,
    COUNT(*) as total_sessions,
    MIN(login_time) as first_login,
    MAX(COALESCE(logout_time, CURRENT_TIMESTAMP)) as last_activity,
    SUM(EXTRACT(EPOCH FROM (COALESCE(logout_time, CURRENT_TIMESTAMP) - login_time))) as total_duration_seconds,
    COALESCE(SUM(gap_seconds), 0) as total_gap_seconds,
    EXTRACT(EPOCH FROM (MAX(COALESCE(logout_time, CURRENT_TIMESTAMP)) - MIN(login_time))) as real_time_span_seconds,
    CASE
        WHEN EXTRACT(EPOCH FROM (MAX(COALESCE(logout_time, CURRENT_TIMESTAMP)) - MIN(login_time))) = 0
            THEN 100.0
        ELSE SUM(EXTRACT(EPOCH FROM (COALESCE(logout_time, CURRENT_TIMESTAMP) - login_time))) * 100.0 /
             EXTRACT(EPOCH FROM (MAX(COALESCE(logout_time, CURRENT_TIMESTAMP)) - MIN(login_time)))
        END as continuity_percentage
FROM chain_data;
END;
$BODY$
LANGUAGE plpgsql;

-- Create indexes if they don't exist
DO
$$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_sessions_user_id') THEN
CREATE INDEX idx_sessions_user_id ON sessions(user_id);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_sessions_login_time') THEN
CREATE INDEX idx_sessions_login_time ON sessions(login_time);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_sessions_logout_time') THEN
CREATE INDEX idx_sessions_logout_time ON sessions(logout_time);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_sessions_continuity') THEN
CREATE INDEX idx_sessions_continuity ON sessions(continued_from_session, continued_by_session);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_tracked_apps_roles_app_id') THEN
CREATE INDEX idx_tracked_apps_roles_app_id ON tracked_applications_roles(app_id);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_tracked_apps_roles_role_id') THEN
CREATE INDEX idx_tracked_apps_roles_role_id ON tracked_applications_roles(role_id);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_tracked_apps_disciplines_app_id') THEN
CREATE INDEX idx_tracked_apps_disciplines_app_id ON tracked_applications_disciplines(app_id);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_tracked_apps_disciplines_discipline_id') THEN
CREATE INDEX idx_tracked_apps_disciplines_discipline_id ON tracked_applications_disciplines(discipline_id);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_afk_periods_session_id') THEN
CREATE INDEX idx_afk_periods_session_id ON afk_periods(session_id);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_afk_periods_timerange') THEN
DROP INDEX IF EXISTS idx_afk_periods_timerange;
CREATE INDEX idx_afk_periods_start_time ON afk_periods(start_time);
CREATE INDEX idx_afk_periods_end_time ON afk_periods(end_time);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_urd_user_id') THEN
CREATE INDEX idx_urd_user_id ON user_role_disciplines(user_id);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_urd_role_id') THEN
CREATE INDEX idx_urd_role_id ON user_role_disciplines(role_id);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_urd_discipline_id') THEN
CREATE INDEX idx_urd_discipline_id ON user_role_disciplines(discipline_id);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_machines_last_seen_at') THEN
CREATE INDEX idx_machines_last_seen_at ON machines(last_seen_at);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_machines_mac_address') THEN
CREATE INDEX idx_machines_mac_address ON machines(mac_address);
END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_indexes WHERE indexname = 'idx_machines_active') THEN
CREATE INDEX idx_machines_active ON machines(active);
END IF;
END
$$;

-- Create or replace views
CREATE OR REPLACE VIEW vw_user_app_permissions AS
SELECT DISTINCT
    u.id as user_id,
    u.name as user_name,
    u.email as user_email,
    r.id as role_id,
    r.code as role_code,
    r.name as role_name,
    d.id as discipline_id,
    d.code as discipline_code,
    d.name as discipline_name,
    a.id as app_id,
    a.app_name,
    a.app_path,
    a.is_restricted,
    a.tracking_enabled
FROM users u
         JOIN user_role_disciplines urd ON u.id = urd.user_id
         JOIN roles r ON urd.role_id = r.id
         JOIN disciplines d ON urd.discipline_id = d.id
         JOIN tracked_applications_roles tar ON r.id = tar.role_id
         JOIN tracked_applications_disciplines tad ON d.id = tad.discipline_id
         JOIN applications a ON tar.app_id = a.id AND tad.app_id = a.id
WHERE a.tracking_enabled = true;

CREATE OR REPLACE VIEW vw_session_active_time AS
WITH session_durations AS (
    SELECT
        s.id as session_id,
        s.user_id,
        u.name as user_name,
        u.email as user_email,
        -- Get role info from user_role_disciplines instead of sessions
        (SELECT r.code FROM user_role_disciplines urd
         JOIN roles r ON urd.role_id = r.id
         WHERE urd.user_id = u.id
         LIMIT 1) as role_code,
        (SELECT r.name FROM user_role_disciplines urd
         JOIN roles r ON urd.role_id = r.id
         WHERE urd.user_id = u.id
         LIMIT 1) as role_name,
        -- Get discipline info from user_role_disciplines instead of sessions
        (SELECT d.code FROM user_role_disciplines urd
         JOIN disciplines d ON urd.discipline_id = d.id
         WHERE urd.user_id = u.id
         LIMIT 1) as discipline_code,
        (SELECT d.name FROM user_role_disciplines urd
         JOIN disciplines d ON urd.discipline_id = d.id
         WHERE urd.user_id = u.id
         LIMIT 1) as discipline_name,
        s.login_time,
        s.logout_time,
        COALESCE(
            EXTRACT(EPOCH FROM (s.logout_time - s.login_time)),
            EXTRACT(EPOCH FROM (CURRENT_TIMESTAMP - s.login_time))
        ) as total_seconds,
        COALESCE(
            SUM(
                EXTRACT(EPOCH FROM (
                    COALESCE(afk.end_time, CURRENT_TIMESTAMP) - afk.start_time
                ))
            ),
            0
        ) as afk_seconds
    FROM sessions s
    JOIN users u ON s.user_id = u.id
    LEFT JOIN afk_periods afk ON s.id = afk.session_id
    GROUP BY s.id, u.id
)
SELECT
    session_id,
    user_id,
    user_name,
    user_email,
    role_code,
    role_name,
    discipline_code,
    discipline_name,
    login_time,
    logout_time,
    total_seconds,
    afk_seconds,
    (total_seconds - afk_seconds) as active_seconds
FROM session_durations;

CREATE OR REPLACE VIEW vw_session_events_summary AS
SELECT
    se.session_id,
    se.event_type,
    se.event_time,
    se.machine_id,
    se.terminal_session_id,
    u.name as user_name,
    u.email as user_email,
    prev_u.name as previous_user_name,
    prev_u.email as previous_user_email,
    se.is_remote,
    se.event_data
FROM session_events se
         JOIN users u ON se.user_id = u.id
         LEFT JOIN users prev_u ON se.previous_user_id = prev_u.id
ORDER BY se.event_time DESC;

CREATE OR REPLACE VIEW vw_maintenance_status AS
SELECT
    task_name,
    status,
    start_time,
    end_time,
    EXTRACT(EPOCH FROM (end_time - start_time))::integer as duration_seconds,
        error_message,
    affected_partitions,
    created_at
FROM maintenance_history
ORDER BY start_time DESC;

CREATE OR REPLACE VIEW session_chains_view AS
WITH RECURSIVE chain_members AS (
    -- Start with sessions that don't have a previous session (root sessions)
    SELECT
        id as session_id,
        user_id,
        machine_id,
        login_time,
        logout_time,
        continued_by_session,
        1 as chain_position,
        id as chain_id
    FROM sessions
    WHERE continued_from_session IS NULL

    UNION ALL

    -- Get all sessions in the chain
    SELECT
        s.id as session_id,
        s.user_id,
        s.machine_id,
        s.login_time,
        s.logout_time,
        s.continued_by_session,
        cm.chain_position + 1,
        cm.chain_id
    FROM sessions s
    JOIN chain_members cm ON s.continued_from_session = cm.session_id
)
SELECT
    chain_id,
    COUNT(session_id) as chain_length,
    MIN(login_time) as chain_start_time,
    MAX(COALESCE(logout_time, CURRENT_TIMESTAMP)) as chain_end_time,
    EXTRACT(EPOCH FROM (MAX(COALESCE(logout_time, CURRENT_TIMESTAMP)) - MIN(login_time))) as total_duration_seconds,
    array_agg(session_id ORDER BY chain_position) as session_ids
FROM chain_members
GROUP BY chain_id
ORDER BY chain_start_time DESC;

-- Insert sample data if not exists
DO
$$
DECLARE
admin_exists boolean;
BEGIN
SELECT EXISTS(SELECT 1 FROM users WHERE email = 'admin@example.com') INTO admin_exists;

IF NOT admin_exists THEN
        INSERT INTO users (
            id,
            name,
            email,
            password,
            active,
            verified,
            created_at,
            updated_at
        ) VALUES (
            gen_random_uuid(),
            'Admin User',
            'admin@example.com',
            'admin123',
            true,
            true,
            CURRENT_TIMESTAMP,
            CURRENT_TIMESTAMP
        );
END IF;

IF NOT EXISTS (SELECT 1 FROM roles WHERE code = 'ADMIN') THEN
        INSERT INTO roles (
            code,
            name,
            description,
            created_at,
            updated_at
        ) VALUES (
            'ADMIN',
            'Administrator',
            'Full system access',
            CURRENT_TIMESTAMP,
            CURRENT_TIMESTAMP
        );
END IF;

    IF NOT EXISTS (SELECT 1 FROM disciplines WHERE code = 'SYSADMIN') THEN
        INSERT INTO disciplines (
            code,
            name,
            description,
            created_at,
            updated_at
        ) VALUES (
            'SYSADMIN',
            'System Administration',
            'IT Administration discipline',
            CURRENT_TIMESTAMP,
            CURRENT_TIMESTAMP
        );
END IF;
END
$$;

-- Create future partitions
SELECT create_future_partitions();

COMMENT ON DATABASE activity_tracker IS 'Activity tracking database with partitioned tables for high-volume data';

-- Add constraints for preventing NULL IDs and other data integrity issues
ALTER TABLE sessions
    ADD CONSTRAINT check_session_id_not_null CHECK (id IS NOT NULL);

ALTER TABLE sessions
    ADD CONSTRAINT check_login_time_not_null CHECK (login_time IS NOT NULL);

ALTER TABLE afk_periods
    ADD CONSTRAINT check_afk_id_not_null CHECK (id IS NOT NULL);

ALTER TABLE afk_periods
    ADD CONSTRAINT check_afk_start_time_not_null CHECK (start_time IS NOT NULL);

-- Add cascade to session foreign keys to help with data cleanup
ALTER TABLE activity_events
DROP CONSTRAINT IF EXISTS activity_events_session_id_fkey,
    ADD CONSTRAINT activity_events_session_id_fkey
    FOREIGN KEY (session_id) REFERENCES sessions(id) ON DELETE CASCADE;

ALTER TABLE afk_periods
DROP CONSTRAINT IF EXISTS afk_periods_session_id_fkey,
    ADD CONSTRAINT afk_periods_session_id_fkey
    FOREIGN KEY (session_id) REFERENCES sessions(id) ON DELETE CASCADE;

ALTER TABLE app_usage
DROP CONSTRAINT IF EXISTS app_usage_session_id_fkey,
    ADD CONSTRAINT app_usage_session_id_fkey
    FOREIGN KEY (session_id) REFERENCES sessions(id) ON DELETE CASCADE;

ALTER TABLE system_metrics
DROP CONSTRAINT IF EXISTS system_metrics_session_id_fkey,
    ADD CONSTRAINT system_metrics_session_id_fkey
    FOREIGN KEY (session_id) REFERENCES sessions(id) ON DELETE CASCADE;

ALTER TABLE session_events
DROP CONSTRAINT IF EXISTS session_events_session_id_fkey,
    ADD CONSTRAINT session_events_session_id_fkey
    FOREIGN KEY (session_id) REFERENCES sessions(id) ON DELETE CASCADE;

-- Token storage table for persistent authentication
CREATE TABLE IF NOT EXISTS auth_tokens (
                                           token_id VARCHAR(64) PRIMARY KEY,   -- The token string (hashed if needed)
    token_type VARCHAR(20) NOT NULL,    -- 'user', 'service', 'refresh', 'api'
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    token_data JSONB NOT NULL,          -- All token metadata
    expires_at TIMESTAMP NOT NULL,      -- Expiration timestamp
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    created_by uuid REFERENCES users(id),
    updated_at TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id),
    revoked BOOLEAN DEFAULT false,      -- For token revocation
    revocation_reason VARCHAR(255),     -- Optional reason for revocation
    device_info JSONB,                  -- Device information
    last_used_at TIMESTAMP              -- Track when token was last used
    );

-- Indexes for token table
CREATE INDEX IF NOT EXISTS idx_auth_tokens_user_id ON auth_tokens(user_id);
CREATE INDEX IF NOT EXISTS idx_auth_tokens_expires_at ON auth_tokens(expires_at);
CREATE INDEX IF NOT EXISTS idx_auth_tokens_revoked ON auth_tokens(revoked) WHERE revoked = false;

