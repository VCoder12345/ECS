@echo off
setlocal

cmake --preset debug 
if errorlevel 1 exit /b %errorlevel%

cmake --build --preset debug

build\debug\tests\ecs_tests.exe

endlocal
