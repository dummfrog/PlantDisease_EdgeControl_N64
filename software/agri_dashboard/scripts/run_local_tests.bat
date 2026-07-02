@echo off
setlocal
cd /d "%~dp0.."

where python >nul 2>nul
if errorlevel 1 (
  echo Python not found.
  exit /b 1
)

python -m py_compile backend\app.py backend\database.py backend\expert_rules.py backend\validators.py tools\test_edge_device_upload.py tools\test_edge_invalid_upload.py tools\load_test_edge_upload.py tools\test_dashboard_e2e.py
if errorlevel 1 exit /b 1

python -m unittest discover -s tests -p "test_*.py"
