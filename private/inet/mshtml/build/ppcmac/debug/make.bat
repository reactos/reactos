@echo off
setlocal
pushd ..\..\..\src
call f3\make\build.bat %1 %2 %3 %4 %5 %6 %7 %8
set _DEBUG=1
set _MACHINE=PPCMAC
set _BLDROOT=$(ROOT)\build\ppcmac\debug
set _PRODUCT=96
set _SYSTEM=mac
set _RELEASE=
set _MACTEXT=
%NMAKE_EXE% %ARG%
popd
@echo on
