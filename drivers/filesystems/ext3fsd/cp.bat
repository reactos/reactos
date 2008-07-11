@echo off
if %1. == . goto help
if %2. == . goto help
ping %1
dir \\%1\%2
build -ceZ
if .%_BUILDARCH% == .AMD64 goto BUILD_X64
if .%_BUILDARCH% == .amd64 goto BUILD_X64
if .%_BUILDARCH% == .Amd64 goto BUILD_X64
if .%_BUILDARCH% == .X86 goto BUILD_X86
if .%_BUILDARCH% == .x86 goto BUILD_X86
goto exit

:BUILD_X64
set TARGET=amd64
set PDB_X64=\X64
goto COPY_BINARY

:BUILD_X86
set TARGET=i386

:COPY_BINARY
copy %DDK_TARGET_OS%\%DDKBUILDENV%\%TARGET%\*.pdb c:\works\symbols%PDB_X64%
copy %DDK_TARGET_OS%\%DDKBUILDENV%\%TARGET%\*.sys \\%1\%2\windows\system32\drivers

REM copy %DDK_TARGET_OS%\%DDKBUILDENV%\%TARGET%\*.sys \\%1\c$\windows\system32\drivers
REM copy %DDK_TARGET_OS%\%DDKBUILDENV%\%TARGET%\*.sys \\%1\c$\symbols
REM copy %DDK_TARGET_OS%\%DDKBUILDENV%\%TARGET%\*.sys \\%1\c$\winnt\system32\drivers
REM call trans . %DDK_TARGET_OS%\%DDKBUILDENV%\%TARGET%\ext2fsd.sys
REM copy %DDK_TARGET_OS%\%DDKBUILDENV%\%TARGET%\*.nms \\%1\%2\Symbols

goto exit
:help
echo cp HostName Driver
:exit
