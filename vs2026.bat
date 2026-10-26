@echo off
setlocal

for %%I in ("%~dp0.") do set "ROOT=%%~fI"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "INSTALL_DIR="

if not exist "%VSWHERE%" (
    echo ERROR: Could not find vswhere.exe. Install Visual Studio 2026 Build Tools or Visual Studio 2026.
    exit /b 1
)

set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
for /f "delims=" %%I in ('vswhere.exe -latest -products * -version "[18.0,19.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath') do (
    if not defined INSTALL_DIR set "INSTALL_DIR=%%I"
)

if not defined INSTALL_DIR (
    echo ERROR: Could not find a Visual Studio 2026 installation with the x64 C++ tools.
    exit /b 1
)

set "VCVARS=%INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat"
set "CMAKE=%INSTALL_DIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if not exist "%VCVARS%" (
    echo ERROR: Could not find "%VCVARS%".
    exit /b 1
)
if not exist "%CMAKE%" (
    echo ERROR: Could not find "%CMAKE%".
    exit /b 1
)

call "%VCVARS%"
if errorlevel 1 (
    echo ERROR: Failed to initialize the Visual Studio 2026 build environment.
    exit /b 1
)

"%CMAKE%" -S "%ROOT%" -B "%ROOT%\build_vs2026" -G "Visual Studio 18 2026" -A x64
if errorlevel 1 (
    echo ERROR: CMake generation failed.
    exit /b 1
)

if /i "%~1"=="--pause" pause
exit /b 0
