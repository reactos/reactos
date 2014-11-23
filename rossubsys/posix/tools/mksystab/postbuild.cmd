@echo off

setlocal

set PATH_TO_TOP=..\..\..\reactos

set PATH_TO_PSX_TOP=..\..

set TARGET_NAME=mksystab

set SYSCALL_DB=syscall.db
set SYSTAB_C=%PATH_TO_PSX_TOP%\server\call\syscall.c
set SYSTAB_H=%PATH_TO_PSX_TOP%\server\include\syscall.h
set SYSCALL_H=%PATH_TO_PSX_TOP%\include\psx\syscall.h
set STUBS_C=%PATH_TO_PSX_TOP%\server\call\stubs.c


mksystab %SYSCALL_DB% %SYSTAB_C% %SYSTAB_H% %SYSCALL_H% %STUBS_C%

endlocal
