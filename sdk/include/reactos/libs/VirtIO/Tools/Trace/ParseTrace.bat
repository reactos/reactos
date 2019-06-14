ECHO off
SETLOCAL EnableDelayedExpansion

SET PDPFile=%1
SET ETLFile=%2

IF [%PDPFile%] == [] (
    GOTO PRINT_HELP_TEXT
)

IF [%ETLFile%] == [] (
    GOTO PRINT_HELP_TEXT
)

IF EXIST "%PDPFile%" (
    IF EXIST "%ETLFile%" (
        tracepdb -f %PDPFile%
        tracefmt.exe -display -p . %ETLFile%
        IF !ERRORLEVEL! NEQ 0 EXIT /B 1
        GOTO EIXT_SCRIPT
    ) ELSE (
        ECHO Please make sure the etl file exists
    )
) ELSE (
    ECHO Please make sure the pdb file exists
)

GOTO EIXT_SCRIPT

:PRINT_HELP_TEXT
ECHO.
ECHO This script parses the collected data from an etl file. The script
ECHO has two parameters in the following order:
ECHO.
ECHO * Path for the symbols file (.pdb) for the sutiable driver build
ECHO * Path for the trace file (.etl) for the sutiable driver
ECHO.
ECHO For the script to run it requires the presence of two tools in the local
ECHO directory; Tracepdb.exe and Tracefmt.exe, both tools can be found in the path:
ECHO C:\Program Files (x86)\Windows Kits\<Kit version>\bin\<ARCH>\

:EIXT_SCRIPT
