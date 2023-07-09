@echo off
rem USAGE:
rem    sign file
rem where,
rem    file      = file to be given a test digital signature

IF '%_TEST%' == '1' @echo on

if '%!BUILDLOG%' == '' set !BUILDLOG=setup.log
if not exist %1 goto END

rem ********SIGN THE FILE*******************************************
%_F3DIR%\build\scripts\tools\%PROCESSOR_ARCHITECTURE%\signcode -spc "trident.spc" -pvk "TridentTestKey" -prog "%1" -name "Microsoft Trident" -info "http://iptdweb.microsoft.com" >> %!BUILDLOG%

:END