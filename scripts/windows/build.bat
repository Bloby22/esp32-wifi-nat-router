@echo off
setlocal

rem Find the project root: walk up from this script until platformio.ini exists.
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
:find_root
if exist "%ROOT%\platformio.ini" goto :root_found
for %%I in ("%ROOT%\..") do set "PARENT=%%~fI"
if /i "%PARENT%"=="%ROOT%" goto :no_root
set "ROOT=%PARENT%"
goto :find_root
:root_found
cd /d "%ROOT%"

set "ACTION=%~1"
set "BOARD=%~2"
if "%ACTION%"=="" set "ACTION=build"
set "ENVARG="
if not "%BOARD%"=="" set "ENVARG=-e %BOARD%"
set "LABEL=%BOARD%"
if "%BOARD%"=="" set "LABEL=the default environment"

rem Find PlatformIO; fall back to its default install location.
where pio >nul 2>&1
if errorlevel 1 set "PATH=%PATH%;%USERPROFILE%\.platformio\penv\Scripts"
where pio >nul 2>&1
if errorlevel 1 goto :no_pio

if /i "%ACTION%"=="build" goto :build
if /i "%ACTION%"=="flash" goto :flash
if /i "%ACTION%"=="monitor" goto :monitor
if /i "%ACTION%"=="clean" goto :clean

echo [ERROR] Unknown action "%ACTION%".
echo Usage: build.bat [build^|flash^|monitor^|clean] [environment]
exit /b 1

:no_root
echo [ERROR] platformio.ini was not found in this folder or any parent folder.
exit /b 1

:no_pio
echo [ERROR] PlatformIO executable "pio" was not found.
echo Install PlatformIO or add %%USERPROFILE%%\.platformio\penv\Scripts to PATH.
exit /b 1

:build
echo === Building firmware for %LABEL% ===
pio run %ENVARG% || exit /b 1
echo === Building LittleFS image for %LABEL% ===
pio run %ENVARG% --target buildfs || exit /b 1
goto :done

:flash
echo === Uploading web files to %LABEL% ===
pio run %ENVARG% --target uploadfs || exit /b 1
echo === Uploading firmware to %LABEL% ===
pio run %ENVARG% --target upload || exit /b 1
echo === Opening serial monitor at 115200 baud ===
pio device monitor -b 115200
goto :done

:monitor
pio device monitor -b 115200
goto :done

:clean
echo === Cleaning %LABEL% ===
pio run %ENVARG% --target clean || exit /b 1
goto :done

:done
echo Done.
exit /b 0