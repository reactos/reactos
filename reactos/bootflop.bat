@ECHO OFF

:
: copy files to HD...
:
COPY /Y A:\DRIVERS\*.SYS C:\reactos\system32\drivers > NUL:
COPY /Y A:\DLLS\*.DLL C:\reactos\system32 > NUL:
COPY /Y A:\APPS\*.EXE C:\reactos\system32 > NUL:
:
: present a menu to the booter...
:
: ECHO 1) IDE,VFatFSD
: ECHO 2) No Drivers
: CHOICE /C:123 /T:1,3 "Select kernel boot config"
: IF ERRORLEVEL 2 GOTO :L2

:L1
CLS
LOADROS NTOSKRNL.EXE IDE.SYS VFATFSD.SYS
GOTO :END

:L2
CLS
LOADROS NTOSKRNL.EXE
GOTO :END

:END
EXIT


