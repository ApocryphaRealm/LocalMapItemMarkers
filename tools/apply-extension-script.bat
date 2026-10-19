@echo off
setlocal
REM Puts swf\IconDisplayExtension.DoAction.as (the readable source of the extension's frame script) back into
REM dist\...\IconDisplayExtension.swf with JPEXS Free Flash Decompiler. Run it after editing that script.
REM
REM FFDEC_CLI may point at ffdec-cli.exe; otherwise ffdec-cli is looked up on PATH.
if "%FFDEC_CLI%"=="" set "FFDEC_CLI=ffdec-cli"

set "SWF=%~dp0..\dist\Interface\InfinityUI\Map\WorldMap\LocalMapMenu\IconDisplayExtension.swf"
set "SRC=%~dp0..\swf\IconDisplayExtension.DoAction.as"

"%FFDEC_CLI%" -replace "%SWF%" "%SWF%.new" "\frame_1\DoAction" "%SRC%"
if errorlevel 1 exit /b 1
move /y "%SWF%.new" "%SWF%" >nul
echo Replaced frame_1 DoAction in %SWF%
