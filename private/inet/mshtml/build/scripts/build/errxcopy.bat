@echo off
rem USAGE:
rem ErrXCopy <src> <dest> <flag(s)>
rem Log the errors to %!copylog%
rem

dir %1 >nul
if errorlevel 1 GOTO ERR

xcopy %1 %2 %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 GOTO ERR

dir %2 >nul
if errorlevel 1 GOTO ERR

goto END

:ERR
echo ERROR WHILE COPYING %1 TO %2
echo ERROR WHILE COPYING %1 TO %2 >>%!COPYLOG%
echo *SOURCE****************************>>%!COPYLOG%
dir %1 >>%!COPYLOG%
echo ***********************************>>%!COPYLOG%

:END
if '%_TEST%' == '1' @echo on