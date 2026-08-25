@echo off
setlocal

set BUILD_DIR=build\benchmark
set BENCH=%BUILD_DIR%\benchmarks\ecs_bench.exe

cmake --preset benchmark
if errorlevel 1 exit /b %errorlevel%

cmake --build --preset benchmark
if errorlevel 1 exit /b %errorlevel%

if "%~1"=="" (
    echo Usage: benchmark.bat ^<output.json^> [benchmark_filter]
    echo.
    echo Example:
    echo   benchmark.bat results\baseline.json
    echo   benchmark.bat results\query.json "BM_Each.*"
    exit /b 1
)

set OUTPUT=%~1
set FILTER=%~2

for %%F in ("%OUTPUT%") do (
    if not exist "%%~dpF" mkdir "%%~dpF"
)

if "%FILTER%"=="" (
    "%BENCH%" ^
        --benchmark_out="%OUTPUT%" 
) else (
    "%BENCH%" ^
        --benchmark_filter="%FILTER%" ^
        --benchmark_out="%OUTPUT%" 
)

endlocal
