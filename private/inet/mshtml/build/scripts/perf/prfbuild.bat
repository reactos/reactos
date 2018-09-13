REM @echo off
REM *PERFBUILD***********************************************

setlocal
SET !BUILDMAIL=/t frm3slm /c benchr
SET !ADMINMAIL=/t sramani

rem _F3dir is already set in system env.
set !MAILOUT=c:\mailout\pending
set !BUILD=%_f3dir%\build\win\ship
set !F3BUILDDIR=c:\build

md %_f3dir%\log
set !SSYNCLOG=%_f3dir%\log\ssync.log
set !BUILDLOG=%_f3dir%\log\build.log
set !ERRORLOG=%_f3dir%\log\error.log

copy %!SSYNCLOG% %!SSYNCLOG%.old
copy %!BUILDLOG% %!BUILDLOG%.old
copy %!ERRORLOG% %!ERRORLOG%.old

del %!SSYNCLOG%
del %!BUILDLOG%
del %!ERRORLOG%

net use /persistent:no
net use \\jeeves\slm /delete
net use \\jeeves\slm /user:REDMOND\frm3bld _frm3bld || goto ERROR

pushd %_F3DIR%
set TIMES=X

REM **************WRITE COOKIE?***************************
:REPEAT
now >>%!ssynclog%
cookie 2>>%!ssynclog%
find "WRITE" %!ssynclog%
if errorlevel 1 goto SSYNC
ECHO Write cookie was found.  Waiting 5 minutes...>>%!ssynclog%
sleep 300
set TIMES=X%TIMES%
if not '%TIMES%' == 'XXXXXXXXXXXX' GOTO REPEAT
set TIMES=X
echo %!BUILDMAIL% /S PERF BLOCKER: WRITE LOCK FOR 1+ HOURS>>%!ssynclog%
copy %!ssynclog% %!mailout%\cookie.txt
del %!ssynclog%
GOTO REPEAT

:SSYNC
REM **************SSYNC************************
cookie -r -c "ssyncing build machine" 2>>%!ssynclog% || goto ERROR
ssync -! -faq -l %!ssynclog% || goto ERROR
cookie -f 2>>%!ssynclog% || goto ERROR
popd

REM **************BUILD************************
pushd %!BUILD%
now >%!buildlog%
call make ffresh >>%!BUILDLOG%
perl %!F3BUILDDIR%\iserror.pl %!BUILDLOG% %!ERRORLOG%
set !errors=No
set !complete=Yes
if not exist %!BUILD%\bin\fm30.dll set !complete=No
if not exist %!BUILD%\bin\fm30pad.exe set !complete=No
if exist %!ERRORLOG% set !errors=Yes
popd

if '%!errors%' == 'No' GOTO NOERRORS
ECHO BUILD FAILURE!! >error.txt
ECHO ***************************************>>error.txt
ECHO Error LOG:  >>error.txt
type %!ERRORLOG% >>error.txt
ECHO ***************************************>>error.txt
ECHO SSYNC LOG:  >>error.txt
type %!SSYNCLOG% >>error.txt
ECHO ***************************************>>error.txt
echo %!BUILDMAIL% /s PERFBUILD COMPILE ERRORS>>error.txt
copy error.txt %!MAILOUT%
del error.txt
goto END

:NOERRORS
if '%!complete%' == 'No' goto ERROR

REM *************PERFTEST**********************
ECHO /s PERFBUILD SUCCESS! /t benchr >>%!MAILOUT%\worked.txt
call \bin\bootNT.bat
GOTO END

REM call \bin\boot95.bat
GOTO END
:ERROR
REM Handle unexpected errors
ECHO UNEXPECTED FAILURE!! >error.txt
ECHO ***************************************>>error.txt
ECHO SSYNC LOG:  >>error.txt
type %!SSYNCLOG% >>error.txt
ECHO ***************************************>>error.txt
ECHO Build LOG:  >>error.txt
type %!BUILDLOG% >>error.txt
ECHO ***************************************>>error.txt
ECHO Error LOG:  >>error.txt
type %!BUILDLOG% >>error.txt
ECHO ***************************************>>error.txt
echo %!ADMINMAIL% /s PERFBUILD UNEXPECTED ERROR>>error.txt
copy error.txt %!MAILOUT%
del error.txt

:END
net use \\jeeves\slm /delete
popd
endlocal