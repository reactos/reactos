rem Set all options to defaults.
set _DEBUG=0
set _MAP=1
set _PRODUCT=96P
set _RELEASE=1
set _NO_INCREMENTAL_LINK=1
set _STDODBC=0
if not "%2" == "" set %2=%3
if not "%4" == "" set %4=%5
set NMAKE_EXE=..\tools\%PROCESSOR_ARCHITECTURE%\bin\nmake.exe
set ARG=%1
if "%ARG%" == "" set ARG=fall
