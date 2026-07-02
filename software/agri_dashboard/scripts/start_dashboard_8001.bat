@echo off
setlocal
cd /d "%~dp0.."

where python >nul 2>nul
if errorlevel 1 (
  echo Python not found. Please install Python or activate the project environment.
  exit /b 1
)

echo Starting local Dashboard on http://127.0.0.1:8001
echo This service is for local Dashboard and PC YOLO demo only.
echo L160/L610 formal uploads must use Supabase Edge Function HTTPS URL.
set DASHBOARD_PORT=8001
python backend\app.py
