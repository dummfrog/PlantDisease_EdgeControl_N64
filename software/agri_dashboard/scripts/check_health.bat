@echo off
setlocal
cd /d "%~dp0.."

where python >nul 2>nul
if errorlevel 1 (
  echo Python not found.
  exit /b 1
)

python -c "import json, urllib.request; data=json.load(urllib.request.urlopen('http://127.0.0.1:8001/api/health', timeout=10)); print(json.dumps(data, ensure_ascii=False, indent=2))"
