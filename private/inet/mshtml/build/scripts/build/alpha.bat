REM @echo off
REM Build on a remote machine

net use \\Trigger\mailout /delete
net use \\FormBuild\forms96$ /delete
net use \\Trigger\f3drop /delete
net use \\f3qa\f3drop /delete
net use \\f3qa\tp3 /delete
net use \\FormBuild\Build$ /delete

:AGAIN
setlocal
rem *************************************************************************
REM Configure HERE:
rem *************************************************************************
net use \\FormBuild\Build$ /user:REDMOND\frm3bld _frm3bld
rem *************************************************************************
set !GoAlpha=\\FormBuild\Build$\GoAlpha.bat
set !mailpath=\\Trigger\mailout\pending
set _destroot=\\Trigger\f3drop
set _flag=A96
rem *************************************************************************

rem Reset the archive bit on everything, so xcopy /m will work
attrib +a *.* /s >nul

Rem set env vars
set r_f3=\\FormBuild\forms96$
set r_src=%r_f3%\src
set r_tools=%r_f3%\tools
set r_help=%r_f3%\help
set r_build=%r_f3%\build\alpha
set !BLDRES=%r_f3%\log\bldres.log
set !NOW=now

@echo off
now
echo Waiting for FormBuild
:loop
sleep 5
if not exist %!GoAlpha% goto loop:
sleep 5
@echo on

rem *************************************************************************
net use \\FormBuild\forms96$ /user:REDMOND\frm3bld _frm3bld
net use \\Trigger\mailout /user:REDMOND\frm3bld _frm3bld
net use \\Trigger\f3drop /user:REDMOND\frm3bld _frm3bld
net use \\f3qa\f3drop /user:REDMOND\frm3bld _frm3bld
net use \\f3qa\tp3 /user:REDMOND\frm3bld _frm3bld
rem *************************************************************************

rem Get Trigger's environment
call %!GoAlpha%
type %!GoAlpha%

rem If clean, delete _entire_ tree
if '%_CLEANMAKE%' == '1' delnode /q %_F3DIR%
if not exist %_f3dir% MD %_f3dir%
pushd %_f3dir%
md log

REM copy current ssync from build machine
xcopy /s /e /k /c /i /f /v %r_src% src>log\ssync.log
xcopy /s /e /k /c /i /f /v %r_tools% tools>>log\ssync.log
xcopy /s /e /k /c /i /f /v %r_help% help>>log\ssync.log
md build
md build\alpha
xcopy /s /e /k /c /i /f /v %r_f3%\build\alpha build\alpha>>log\ssync.log
xcopy %r_f3%\version.h>>log\ssync.log

rem Make destination, just in case
set _destdir=%_destroot%\%_DROPNAME%

REM Normally, don't create it, it'll cause the other build to make a .NEW
REM if not exist %_destdir% goto ERR
if not exist %_destdir% md %_destdir%

if not exist %_destdir%\build md %_destdir%\build
if not exist %_destdir%\build\alpha md %_destdir%\build\alpha
if not exist %_destdir%\build\alpha\log md %_destdir%\build\alpha\log

REM /k wait  /c don't
start /wait cmd.exe /c %_f3qadir%\build\bldcore2 alpha\ship  alpha ship  96
start /wait cmd.exe /c %_f3qadir%\build\bldcore2 alpha\debug alpha debug 96

goto DONE

:ERR
echo Build failed!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

:DONE
del %!GoAlpha%
popd
endlocal
net use \\Trigger\mailout /delete
net use \\FormBuild\forms96$ /delete
net use \\Trigger\f3drop /delete
net use \\f3qa\f3drop /delete
net use \\f3qa\tp3 /delete
net use \\FormBuild\Build$ /delete
goto AGAIN
