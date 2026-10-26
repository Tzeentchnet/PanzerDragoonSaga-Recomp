@echo off
setlocal

for %%I in ("%~dp0.") do set "ROOT=%%~fI"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "INSTALL_DIR="

if not exist "%VSWHERE%" (
    echo ERROR: Could not find vswhere.exe. Install Visual Studio 2022 Build Tools or Visual Studio 2022.
    exit /b 1
)

set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
for /f "delims=" %%I in ('vswhere.exe -latest -products * -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath') do (
    if not defined INSTALL_DIR set "INSTALL_DIR=%%I"
)

if not defined INSTALL_DIR (
    echo ERROR: Could not find a Visual Studio 2022 installation with the x64 C++ tools.
    exit /b 1
)

set "VCVARS=%INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
    echo ERROR: Could not find "%VCVARS%".
    exit /b 1
)

call "%VCVARS%"
if errorlevel 1 (
    echo ERROR: Failed to initialize the Visual Studio 2022 build environment.
    exit /b 1
)

where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake is not available on PATH.
    exit /b 1
)

cmake -S "%ROOT%" -B "%ROOT%\build_vs2022" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo ERROR: CMake generation failed.
    exit /b 1
)

if /i "%~1"=="--pause" pause
exit /b 0
