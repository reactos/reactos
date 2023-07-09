@echo off

set root=%_ntdrive%%_ntroot%\private\shell\ext\dsui
set dest=%1
if "%dest%"=="" set dest=c:\public\test

set processor=%processor_architecture%
if %processor%==x86 set processor=i386

md %dest%

xcopy %root%\dsfolder\winnt\obj\%processor%\dsfolder.dll %dest%
xcopy %root%\cmnquery\winnt\obj\%processor%\cmnquery.dll %dest%
xcopy %root%\dsquery\winnt\obj\%processor%\dsquery.dll   %dest%
xcopy %root%\dsuiext\winnt\obj\%processor%\dsuiext.dll   %dest%
