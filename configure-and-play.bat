@echo off
setlocal

set "SCRIPT=%~dp0tools\pds-config.ps1"
if not exist "%SCRIPT%" (
    echo ERROR: Configuration tool not found: "%SCRIPT%"
    exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -STA -File "%SCRIPT%" %*
exit /b %ERRORLEVEL%
