@echo off
setlocal
pushd ..\..\..\src
call f3\make\build.bat %1 %2 %3 %4 %5 %6 %7 %8
set _MAP=1
set _DEBUG=3
set _MACHINE=x86
set _BLDROOT=$(ROOT)\build\mips\profile
set _PRODUCT=96
%NMAKE_EXE% %ARG%
popd
@echo on
