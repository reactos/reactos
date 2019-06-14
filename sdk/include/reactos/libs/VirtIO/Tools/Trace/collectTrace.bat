ECHO off
SETLOCAL EnableDelayedExpansion

SET Filename=%1.etl
SET Guid=%2
SET DataCollector="virtiowin"

IF [%Filename%]==[] (
    GOTO PRINT_HELP_TEXT
)
IF [%Guid%]==[] (
    GOTO PRINT_HELP_TEXT
)

:COLLECT_TRACE
PUSHD "%~dp0"
logman stop %DataCollector% -ets >NUL 2>&1
logman delete %DataCollector% -ets >NUL 2>&1
logman create trace %DataCollector% -o %Filename% -ow -ets
IF !ERRORLEVEL! NEQ 0 EXIT /B 1
logman update %DataCollector% -p %Guid% 0x7fffffff 6 -ets
IF !ERRORLEVEL! NEQ 0 EXIT /B 1
ECHO Recording started.
ECHO Reproduce the problem, then press ENTER
PAUSE > NUL
logman stop %DataCollector% -ets
DIR %Filename%
ECHO Please collect %Filename% file now
PAUSE
GOTO EIXT_SCRIPT

:PRINT_HELP_TEXT
ECHO.
ECHO This script collects data from the wpp trace prints without the
ECHO need of any external files. It must be executed with administrator
ECHO priviledges and provided with an output filename for the etl file and
ECHO the GUID fot the WPP trace provider of the driver as parameters in this
ECHO order.

:EIXT_SCRIPT
POPD