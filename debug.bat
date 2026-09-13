@echo off
setlocal

cmake --preset debug 
if errorlevel 1 exit /b %errorlevel%

cmake --build --preset debug
if errorlevel 1 exit /b %errorlevel%

build\debug\tests\ecs_tests.exe

endlocal
