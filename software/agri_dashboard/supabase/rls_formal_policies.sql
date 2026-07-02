-- Formal RLS policy plan for STM32N647 device upload.
-- Review and run in Supabase SQL Editor when switching from demo mode to formal mode.
--
-- Demo mode used broad anon/authenticated INSERT policies so the local Flask
-- dashboard and quick demos could write directly through the Data API.
--
-- Formal mode:
-- 1. L160/L610 devices never write with anon keys.
-- 2. Devices POST JSON to Supabase Edge Function device-upload.
-- 3. Edge Function validates device_token and inserts with service role secret.
-- 4. device_token is never stored in diagnosis_records.
-- 5. Historical data is not deleted or truncated by this script.

alter table public.diagnosis_records enable row level security;
alter table public.action_records enable row level security;
alter table public.device_registry enable row level security;
alter table public.device_telemetry enable row level security;
alter table public.actuation_logs enable row level security;
alter table public.security_events enable row level security;
alter table public.model_versions enable row level security;

-- Make Data API exposure explicit. Supabase is moving toward explicit grants
-- for public schema access, so keep grants reviewable in SQL.
grant select on public.diagnosis_records to anon, authenticated;
grant select on public.action_records to anon, authenticated;
grant select on public.dashboard_recent_records_v to anon, authenticated;
grant select on public.dashboard_node_status_v to anon, authenticated;
grant select on public.dashboard_risk_summary_v to anon, authenticated;
grant select on public.dashboard_heatmap_24h_v to anon, authenticated;
grant select on public.dashboard_alerts_v to anon, authenticated;
grant select (device_id, display_name, region_name, crop_type, is_active, last_seen_at, created_at, updated_at)
  on public.device_registry to anon, authenticated;
grant select (model_name, model_type, class_count, class_mapping_json, accuracy, is_active, created_at)
  on public.model_versions to anon, authenticated;

-- Formal device writes are service-role-only via Edge Function.
-- Existing service_role bypasses RLS, but grants keep access explicit.
grant select, insert on public.diagnosis_records to service_role;
grant select, insert on public.action_records to service_role;
grant select, insert, update on public.device_registry to service_role;
grant select, insert on public.device_telemetry to service_role;
grant select, insert on public.actuation_logs to service_role;
grant select, insert on public.security_events to service_role;
grant select, insert, update on public.model_versions to service_role;
grant usage, select on sequence public.diagnosis_records_id_seq to service_role;
grant usage, select on sequence public.action_records_id_seq to service_role;
grant usage, select on all sequences in schema public to service_role;

-- Remove demo-wide anonymous inserts.
drop policy if exists demo_insert_diagnosis_records on public.diagnosis_records;
drop policy if exists demo_insert_action_records on public.action_records;

revoke insert on public.diagnosis_records from anon, authenticated;
revoke insert on public.action_records from anon, authenticated;
revoke insert, update, delete on public.device_registry from anon, authenticated;
revoke insert, update, delete on public.device_telemetry from anon, authenticated;
revoke insert, update, delete on public.actuation_logs from anon, authenticated;
revoke all on public.security_events from anon, authenticated;
revoke insert, update, delete on public.model_versions from anon, authenticated;

-- Keep minimal read access for the current local Dashboard path when it still
-- reads through a backend configured with a publishable/anon key.
-- If the Flask backend is later moved to a service role or a private backend
-- API, these anon select grants and policies can be removed.
drop policy if exists demo_read_diagnosis_records on public.diagnosis_records;
create policy dashboard_read_recent_diagnosis_records
on public.diagnosis_records
for select
to anon, authenticated
using (true);

drop policy if exists demo_read_action_records on public.action_records;
create policy dashboard_read_recent_action_records
on public.action_records
for select
to anon, authenticated
using (true);

drop policy if exists dashboard_read_active_device_registry on public.device_registry;
create policy dashboard_read_active_device_registry
on public.device_registry
for select
to anon, authenticated
using (is_active = true);

drop policy if exists dashboard_read_model_versions on public.model_versions;
create policy dashboard_read_model_versions
on public.model_versions
for select
to anon, authenticated
using (is_active = true);

-- No anon/authenticated policies are created for security_events. It is only
-- written and read by Edge Function/server-side tooling with service role.

-- Optional stricter production step:
-- If Dashboard no longer needs anon Data API reads, run these lines instead
-- after updating the Flask backend to use service role or a server-only API:
--
-- drop policy if exists dashboard_read_recent_diagnosis_records on public.diagnosis_records;
-- drop policy if exists dashboard_read_recent_action_records on public.action_records;
-- revoke select on public.diagnosis_records from anon, authenticated;
-- revoke select on public.action_records from anon, authenticated;
