@echo off
cmake -DCFD_ACTION=clean -P "%~dp0CMakeLists.txt"
exit /b %errorlevel%
