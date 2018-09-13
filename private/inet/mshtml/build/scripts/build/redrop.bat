REM @echo off
if '%1' == '' goto USAGE

if not exist \\trigger\trident\%1       MD \\trigger\trident\%1
if not exist \\trigger\trident\%1 goto ERROR

if '%_f3dir%' == '' set _f3dir=d:\forms3
if not exist %_f3dir%\version.h goto ERROR

set _DROPNAME=%1
pushd %_f3dir%\build
call drop win\ship win ship \\trigger\trident\%1
call drop win\debug win debug \\trigger\trident\%1

goto END
:ERROR
echo An ERROR has been detected!
echo.
:USAGE
echo.
echo USAGE:
echo.
echo REDROP (dropname)
echo.

:END
popd