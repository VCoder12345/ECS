@echo off
setlocal

cmake --preset profiling 
if errorlevel 1 exit /b %errorlevel%

cmake --build --preset profiling

endlocal
