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
    last_known_ip INET,
    last_seen_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP NOT NULL,
    active BOOLEAN DEFAULT true NOT NULL,
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP NOT NULL,
    created_by uuid REFERENCES users(id),
    updated_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP NOT NULL,
    updated_by uuid REFERENCES users(id),
    CONSTRAINT machine_unique_id_unique UNIQUE (machine_unique_id)
    );

-- Create indexes
CREATE INDEX IF NOT EXISTS idx_machines_last_seen_at ON machines(last_seen_at);
CREATE INDEX IF NOT EXISTS idx_machines_mac_address ON machines(mac_address);
CREATE INDEX IF NOT EXISTS idx_machines_active ON machines(active);

-- Modify sessions table to reference machines
ALTER TABLE sessions DROP COLUMN IF EXISTS machine_id;
ALTER TABLE sessions ADD COLUMN machine_id uuid REFERENCES machines(id);

-- Modify session_events table to reference machines
ALTER TABLE session_events DROP COLUMN IF EXISTS machine_id;
ALTER TABLE session_events ADD COLUMN machine_id uuid REFERENCES machines(id);