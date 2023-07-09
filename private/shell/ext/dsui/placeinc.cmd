@echo off

set dest=%1
if "%dest%"=="" set dest=c:\public\test

md %dest%\inc

xcopy \nt\public\sdk\inc\dsclient.h     %dest%\inc
xcopy \nt\public\sdk\inc\dsquery.h      %dest%\inc
xcopy \nt\public\sdk\inc\cmnquery.h     %dest%\inc

xcopy \nt\private\windows\inc\dsclintp.h        %dest%\inc
xcopy \nt\private\windows\inc\dsqueryp.h        %dest%\inc
xcopy \nt\private\windows\inc\cmnquryp.h        %dest%\inc
