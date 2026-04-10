@echo off
REM ============================================
REM  Slate Build Script (MinGW + Qt 6.11.0)
REM ============================================

REM Navigate to the script's own directory
cd /d "%~dp0"
echo [Slate] Working directory: %cd%

:: **NOTE : UPDATE THE PATHS BELOW TO MATCH YOUR INSTALLATION**
set QT_PATH=C:\Qt\6.11.0\mingw_64 
set MINGW_PATH=C:\Qt\Tools\mingw1310_64
set CMAKE_PATH=C:\Qt\Tools\CMake_64\bin
set NINJA_PATH=C:\Qt\Tools\Ninja

REM Add everything to PATH
set PATH=%CMAKE_PATH%;%NINJA_PATH%;%MINGW_PATH%\bin;%QT_PATH%\bin;%PATH%

echo [Slate] Qt:    %QT_PATH%
echo [Slate] MinGW: %MINGW_PATH%
echo [Slate] CMake: %CMAKE_PATH%
echo.

REM Check tools
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found!
    echo         Checking for CMake in Qt Tools...
    dir "C:\Qt\Tools" /b /ad
    echo.
    echo         Update CMAKE_PATH in build.bat to match your installation.
    pause
    exit /b 1
)

where g++ >nul 2>&1
if errorlevel 1 (
    echo [ERROR] g++ not found!
    echo         Update MINGW_PATH in build.bat.
    pause
    exit /b 1
)

cmake --version
echo.

REM Configure
echo [Slate] Configuring...
cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QT_PATH%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [ERROR] CMake configure failed!
    pause
    exit /b 1
)

REM Build
echo.
echo [Slate] Building...
cmake --build build --config Release -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [Slate] Build successful!

REM Deploy Qt DLLs
echo [Slate] Deploying Qt dependencies...
%QT_PATH%\bin\windeployqt.exe build\Slate.exe --no-translations 2>nul

echo.
echo [Slate] Done! Run: build\Slate.exe
pause
