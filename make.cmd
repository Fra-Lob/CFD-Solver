@echo off
if "%~1"=="" goto build
if /I "%~1"=="clean" goto clean
if /I "%~1"=="debug" goto debug
echo Usage: make [debug^|clean]
exit /b 1
:build
cmake -DCFD_ACTION=build -P "%~dp0CMakeLists.txt"
exit /b %errorlevel%
:debug
cmake -DCFD_ACTION=debug -P "%~dp0CMakeLists.txt"
exit /b %errorlevel%
:clean
call "%~dp0clean.cmd"
exit /b %errorlevel%
