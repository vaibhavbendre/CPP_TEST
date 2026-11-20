@echo off
rem Double-click this file to start the movie ticket booking program.
title Movie Ticket Booking
set "EXE=%~dp0build\cinema_cli.exe"
if not exist "%EXE%" set "EXE=%~dp0cinema_cli.exe"
if not exist "%EXE%" (
  echo Could not find cinema_cli.exe. Please read HOW_TO_RUN.md.
  pause
  exit /b 1
)
"%EXE%"
echo.
pause
