@echo off
setlocal

for %%I in ("%~dp0.") do set "ROOT=%%~fI"
set "MODE=%~1"
if not defined MODE set "MODE=Debug"

set "CONFIGURE_PRESET="
set "BUILD_PRESET="
set "TEST_PRESET="

if /I "%MODE%"=="Debug" (
    set "CONFIGURE_PRESET=windows-msvc-x64"
    set "BUILD_PRESET=windows-msvc-x64-debug"
    set "TEST_PRESET=windows-msvc-x64-debug"
)
if /I "%MODE%"=="Release" (
    set "CONFIGURE_PRESET=windows-msvc-x64"
    set "BUILD_PRESET=windows-msvc-x64-release"
    set "TEST_PRESET=windows-msvc-x64-release"
)
if /I "%MODE%"=="Shipping" (
    set "CONFIGURE_PRESET=windows-msvc-x64"
    set "BUILD_PRESET=windows-msvc-x64-shipping"
    set "TEST_PRESET=windows-msvc-x64-shipping"
)
if /I "%MODE%"=="Asan" (
    set "CONFIGURE_PRESET=windows-msvc-x64-asan"
    set "BUILD_PRESET=windows-msvc-x64-asan"
    set "TEST_PRESET=windows-msvc-x64-asan"
)
if /I "%MODE%"=="Tracy" (
    set "CONFIGURE_PRESET=windows-msvc-x64-tracy"
    set "BUILD_PRESET=windows-msvc-x64-tracy"
    set "TEST_PRESET=windows-msvc-x64-tracy"
)

if not defined CONFIGURE_PRESET (
    echo ERROR: Unknown build mode "%MODE%".
    echo Supported core modes: Debug, Release, Shipping.
    echo Optional diagnostics: Asan, Tracy.
    exit /b 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "INSTALL_DIR="

if not exist "%VSWHERE%" (
    echo ERROR: Could not find vswhere.exe. Install Visual Studio Build Tools with the x64 C++ tools.
    exit /b 1
)

set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
for /f "delims=" %%I in ('vswhere.exe -latest -products * -version "[17.0,19.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath') do (
    if not defined INSTALL_DIR set "INSTALL_DIR=%%I"
)

if not defined INSTALL_DIR (
    echo ERROR: Could not find a supported Visual Studio Build Tools installation with the x64 C++ tools.
    exit /b 1
)

set "VCVARS=%INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
    echo ERROR: Could not find "%VCVARS%".
    exit /b 1
)

call "%VCVARS%" >nul
if not "%ERRORLEVEL%"=="0" (
    echo ERROR: Failed to initialize the MSVC x64 build environment.
    exit /b 1
)

where cmake >nul 2>&1
if not "%ERRORLEVEL%"=="0" (
    echo ERROR: CMake 4.4 or newer is not available on PATH.
    exit /b 1
)

where ninja >nul 2>&1
if not "%ERRORLEVEL%"=="0" (
    echo ERROR: Ninja 1.13 or newer is not available on PATH.
    exit /b 1
)

pushd "%ROOT%"
if not "%ERRORLEVEL%"=="0" (
    echo ERROR: Could not enter repository root "%ROOT%".
    exit /b 1
)

set "EXIT_CODE=0"

cmake --preset "%CONFIGURE_PRESET%"
if not "%ERRORLEVEL%"=="0" (
    echo ERROR: CMake configuration failed.
    set "EXIT_CODE=1"
    goto finish
)

cmake --build --preset "%BUILD_PRESET%"
if not "%ERRORLEVEL%"=="0" (
    echo ERROR: Build failed for mode "%MODE%".
    set "EXIT_CODE=1"
    goto finish
)

ctest --preset "%TEST_PRESET%"
if not "%ERRORLEVEL%"=="0" (
    echo ERROR: Tests failed for mode "%MODE%".
    set "EXIT_CODE=1"
    goto finish
)

:finish
popd
exit /b %EXIT_CODE%
