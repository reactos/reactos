@echo off

set root=%_ntdrive%%_ntroot%\private\shell\ext\dsui
set dest=%1
if "%dest%"=="" set dest=c:\public\test95

set processor=%processor_architecture%
if %processor%==x86 set processor=i386

md %dest%

xcopy %root%\dsfolder\win95\obj\%processor%\dsfolder.dll %dest%
xcopy %root%\cmnquery\win95\obj\%processor%\cmnquery.dll %dest%
xcopy %root%\dsquery\win95\obj\%processor%\dsquery.dll   %dest%
xcopy %root%\dsuiext\win95\obj\%processor%\dsuiext.dll   %dest%
