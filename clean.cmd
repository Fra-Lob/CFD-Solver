@echo off
rem Windows entry point for the plain clean command.
cmake -DCFD_ACTION=clean -P "%~dp0CMakeLists.txt"
exit /b %errorlevel%
