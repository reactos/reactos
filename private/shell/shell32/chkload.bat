@echo off
if "%OS%"=="Windows_NT" goto nodel
%1\dev\tools\binw\x86\oink32.exe -v4 %2 | %1\dev\tools\binr\fgrep Fixup
if errorlevel 1 goto nodel
echo **************************************************************
echo ERROR!                     ERROR!                       ERROR!
echo YOU HAVE FIXUPS IN THE SHARED SEGMENT. DELETING %2.
echo **************************************************************
del %2
exit 1
:nodel
exit 0
