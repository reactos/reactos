@echo off
if not "%1"=="" set _ntdrive=%1
if not "%2"=="" set _ntroot=%2
if "%_ntdrive%"=="" set _ntdrive=c:
if "%_ntroot%"=="" set _ntroot=\nt
set _ntbindir=%_ntdrive%%_ntroot%
set _ieroot=%_ntroot%
if not "%processor_architecture%" == "x86" set build_default_targets=-%processor_architecture%

set PATH=%SystemRoot%;%SystemRoot%\system32;%_NTDRIVE%%_NTROOT%\idw;%_NTDRIVE%%_NTROOT%\mstools;%_NTDRIVE%%_NTROOT%\bin;%_NTDRIVE%%_NTROOT%\bin\%PROCESSOR_ARCHITECTURE%;%PATH%
set msxmlenv=1

:setenv
set ntdebug=ntsd
set ntdebugtype=both
set ntdbgfiles=1
set msc_optimization=/Odi
set BUILD_OPTIONS=%BUILD_OPTIONS% -w
set FULL_DEBUG=1

if "%OS%" == "Windows_NT" goto useparam3
if "%use_msxml_incremental_linking%"=="" set use_msxml_incremental_linking=%4
goto default
:useparam3
if "%use_msxml_incremental_linking%"=="" set use_msxml_incremental_linking=%3
:default
if "%use_msxml_incremental_linking%"=="" set use_msxml_incremental_linking=1
setlocal
set mkfile=i386mk.inc
set incfile=msxmlenv.inc
if not "%processor_architecture%" == "x86" set mkfile=alphamk.inc
if not "%processor_architecture%" == "x86" set incfile=msxmlena.inc
diff %_ntdrive%%_ntroot%\private\inet\msxml\%incfile% %_ntdrive%%_ntroot%\public\oak\bin\%mkfile% >nul
if errorlevel 1 goto different
goto done

:different
echo Updating %_ntdrive%%_ntroot%\public\oak\bin\%mkfile%
cd %_ntdrive%%_ntroot%\public\oak\bin
out -!fz %mkfile%
copy %_ntdrive%%_ntroot%\private\inet\msxml\%incfile% %_ntdrive%%_ntroot%\public\oak\bin\%mkfile% >nul
endlocal

:done
call %_ntdrive%%_ntroot%\private\developr\%username%\razzle.cmd