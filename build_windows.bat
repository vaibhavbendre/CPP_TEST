@echo off
rem Builds cinema_cli.exe and runs the tests on Windows. Needs: Visual Studio 2022 (with
rem "Desktop development with C++"). Only people who BUILD need this; people who just
rem RUN the program use run_cinema.bat and the ready-made cinema_cli.exe.
setlocal
set "VS="
for %%E in (Community Professional Enterprise BuildTools) do (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" set "VS=%ProgramFiles%\Microsoft Visual Studio\2022\%%E"
)
if not defined VS (echo Visual Studio 2022 was not found. & exit /b 1)
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "CM=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake"
set "PATH=%CM%\CMake\bin;%CM%\Ninja;%PATH%"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release || exit /b 1
cmake --build build || exit /b 1
ctest --test-dir build --output-on-failure || exit /b 1
echo.
echo Build OK. Start the program with run_cinema.bat
