-- Crear tablas del sistema de acceso
-- Ejecutar en Supabase SQL Editor

CREATE TABLE authorized_cards (
    uid_hex TEXT PRIMARY KEY,
    finger_id INTEGER NOT NULL,
    name TEXT DEFAULT '',
    active BOOLEAN DEFAULT true,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE access_logs (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    uid_hex TEXT NOT NULL,
    success BOOLEAN NOT NULL,
    reason TEXT DEFAULT '',
    finger_id INTEGER DEFAULT 0,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- RLS: permitir anon key para INSERT/SELECT
ALTER TABLE authorized_cards ENABLE ROW LEVEL SECURITY;
ALTER TABLE access_logs ENABLE ROW LEVEL SECURITY;

CREATE POLICY "anon_all" ON authorized_cards
    FOR ALL TO anon USING (true) WITH CHECK (true);

CREATE POLICY "anon_insert" ON access_logs
    FOR INSERT TO anon WITH CHECK (true);

CREATE POLICY "anon_select" ON access_logs
    FOR SELECT TO anon USING (true);
