-- Crear tablas del sistema de acceso
-- Ejecutar en Supabase SQL Editor

CREATE TABLE IF NOT EXISTS authorized_cards (
    uid_hex TEXT PRIMARY KEY,
    finger_id INTEGER NOT NULL,
    name TEXT DEFAULT '',
    active BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT (NOW() AT TIME ZONE 'America/Bogota')
);

CREATE TABLE IF NOT EXISTS access_logs (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    uid_hex TEXT NOT NULL,
    success BOOLEAN NOT NULL,
    reason TEXT DEFAULT '',
    finger_id INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT (NOW() AT TIME ZONE 'America/Bogota')
);

-- RLS: permitir anon key para INSERT/SELECT
ALTER TABLE authorized_cards ENABLE ROW LEVEL SECURITY;
ALTER TABLE access_logs ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "anon_all" ON authorized_cards;
CREATE POLICY "anon_all" ON authorized_cards
    FOR ALL TO anon USING (true) WITH CHECK (true);

DROP POLICY IF EXISTS "anon_insert" ON access_logs;
CREATE POLICY "anon_insert" ON access_logs
    FOR INSERT TO anon WITH CHECK (true);

DROP POLICY IF EXISTS "anon_select" ON access_logs;
CREATE POLICY "anon_select" ON access_logs
    FOR SELECT TO anon USING (true);

-- Migración desde schema anterior (TIMESTAMPTZ → TIMESTAMP Bogota)
ALTER TABLE authorized_cards ALTER COLUMN created_at TYPE TIMESTAMP,
  ALTER COLUMN created_at SET DEFAULT (NOW() AT TIME ZONE 'America/Bogota');
ALTER TABLE access_logs ALTER COLUMN created_at TYPE TIMESTAMP,
  ALTER COLUMN created_at SET DEFAULT (NOW() AT TIME ZONE 'America/Bogota');

-- Tabla para comandos remotos (desbloqueo desde web)
CREATE TABLE IF NOT EXISTS pending_commands (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    command TEXT NOT NULL,
    params JSONB DEFAULT '{}',
    created_at TIMESTAMP DEFAULT (NOW() AT TIME ZONE 'America/Bogota'),
    executed BOOLEAN DEFAULT false
);
ALTER TABLE pending_commands ENABLE ROW LEVEL SECURITY;
DROP POLICY IF EXISTS "anon_all" ON pending_commands;
CREATE POLICY "anon_all" ON pending_commands
    FOR ALL TO anon USING (true) WITH CHECK (true);
