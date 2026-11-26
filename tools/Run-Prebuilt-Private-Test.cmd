@echo off
setlocal
cd /d "%~dp0"
if not exist "awe32.raw" (
  echo Place your own original awe32.raw next to this CMD file first.
  pause
  exit /b 1
)
where python >nul 2>&1 || (echo Python 3 is required.& pause & exit /b 1)
where git >nul 2>&1 || (echo Git is required.& pause & exit /b 1)
powershell.exe -NoProfile -File "%~dp0tools\Run-Private-Creative-AWE-Test.ps1" -Bundle "%~dp0125A AWE64 Gold.vst3" -HostExe "%~dp0awe64_vst3_host_pcm_smoke.exe" -Rom "%~dp0awe32.raw" -Seconds 60
set "RESULT=%ERRORLEVEL%"
echo Private original Creative test exit code: %RESULT%
pause
exit /b %RESULT%
