@echo off

REM     9.12.96         Bill Ritchie            "New script, gets disk space quota from ITG Managed Servers"

Path=%homedrive%\qminq;%path%

@if "%1"=="" goto syntax

net use x: /d

@if "%1"=="ddtdrop" net use x: \\brant\%1
@if "%1"=="Formroot" net use x: \\stork\%1

@echo %1

@if not exist %homedrive%\qminq\qminq32.exe goto error

@if exist %homedrive%\qminq\qminq32.exe call qminq32.exe && goto end

@if not exist %homedrive%\qminq\qminq16.exe goto error

@if exist %homedrive%\qminq\qminq32.exe call qminq16.exe && goto end

:error

@echo "You do not have the ITG QUOTA Tools on your system.
goto end

:syntax

@echo "Note" "Only used on ITG Managed Server Shares"

@echo QUOTA [%%1]
@echo QUOTA [SHARE NAME]
@echo QUOTA [DDTdrop] 
@echo QUOTA [Formroot] 
goto end

:end
