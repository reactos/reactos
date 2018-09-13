rem echo off
if %_TARGET%==ia64 goto ia64
%BINARIES%\xfb\mstools\cl -nologo -Iia64\ -I.  -I.. -I..\..\inc -I%_NTBINDIR%\public\oak\inc -I%_NTBINDIR%\public\sdk\inc -I%_NTBINDIR%\public\sdk\inc\crt -D_GENIA64_ -D__unaligned="" -U_M_IX86 -D_M_IA64=1 -D_IA64_=1 -DIA64=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DWIN32_LEAN_AND_MEAN=1 -DNT_UP=1  -DNT_INST=0 -DWIN32=100 -D_NT1X_=100  -DDBG=0 -DDEVL=1 -DFPO=1  -D_IDWBUILD -DRDRDBG -DSRVDBG  -D_NTSYSTEM_   /Ze /Zp8 /Gy -cbstring /W3 /Oxs /Oy /G4 genia64.c /MD /link %_NTBINDIR%\public\sdk\lib\i386\msvcrt.lib %_NTBINDIR%\public\sdk\lib\i386\advapi32.lib %_NTBINDIR%\public\sdk\lib\i386\kernel32.lib %_NTBINDIR%\public\sdk\lib\i386\ntdll.lib 
goto end

:ia64
set _path_=%path%
set path=%NT_ROOT%\Sp2Tools;%path%
cl -nologo -Iia64\ -I.  -I.. -I..\..\inc -I%_NTBINDIR%\public\oak\inc -I%_NTBINDIR%\public\sdk\inc -I%_NTBINDIR%\public\sdk\inc\crt -D__unaligned="" -D_IA64_=1 -DIA64=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DWIN32_LEAN_AND_MEAN=1 -DNT_UP=1  -DNT_INST=0 -DWIN32=100 -D_NT1X_=100  -DDBG=0 -DDEVL=1 -DFPO=1  -D_IDWBUILD -DRDRDBG -DSRVDBG  -D_NTSYSTEM_   /Ze /Zp8 /Gy -cbstring /W3 /Od /Ap64 /Oy /c genia64.c

ilink /lp64 /nodefaultlib /machine:ia64 /out:genia64.exe genia64.obj %NT_ROOT%\Sp2Tools\libcEM.lib %NT_ROOT%\Sp2Tools\libmEM.lib

gambit genia64.exe

set path=%_path_%

: end

