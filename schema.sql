-- ============================================================
--  ValveGuard – Supabase PostgreSQL Schema
--  Run this in your Supabase project → SQL Editor
-- ============================================================

-- 1. Create the devices table
DROP TABLE IF EXISTS public.devices CASCADE;
CREATE TABLE IF NOT EXISTS public.devices (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES auth.users(id) ON DELETE CASCADE,
    phone_number TEXT NOT NULL,
    device_code TEXT UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

-- 2. Enable Row-Level Security (RLS)
ALTER TABLE public.devices ENABLE ROW LEVEL SECURITY;

-- 3. Policy: anyone authenticated can read their own devices
CREATE POLICY "Allow authenticated reads for own devices"
  ON public.devices
  FOR SELECT
  TO authenticated
  USING (auth.uid() = user_id);

-- 4. Policy: backend service-role can insert/update devices
--    (the backend uses the service-role key so it bypasses RLS automatically)

-- 5. Optional: index for faster ordering by time
CREATE INDEX IF NOT EXISTS devices_created_at_idx
  ON public.devices (created_at DESC);

-- Done! The table is ready.
