@ECHO OFF

:
: copy files to HD...
:
COPY /Y SHELL.BIN C:\reactos\system\SHELL.bin
COPY /Y BLUES.o C:\reactos\system\drivers\BLUES.o
COPY /Y KEYBOARD.o C:\reactos\system\drivers\KEYBOARD.o

:
: present a menu to the booter...
:
ECHO 1) Keyboard,IDE,VFatFSD
ECHO 2) IDE,VFatFSD
ECHO 3) No Drivers
CHOICE /C:123 /T:2,10 "Select kernel boot config"
IF ERRORLEVEL 3 GOTO :L3
IF ERRORLEVEL 2 GOTO :L2

:L1
CLS
LOADROS KIMAGE.BIN KEYBOARD.O IDE.O VFATFSD.O
GOTO :END

:L2
CLS
LOADROS KIMAGE.BIN IDE.O VFATFSD.O
GOTO :END

:L3
CLS
LOADROS KIMAGE.BIN
GOTO :END

:END
EXIT


