@echo off
rem
rem This script requires perl.exe
rem


cat %_NTDRIVE%%_NTROOT%\private\genx\windows\inc\winuser.w %_NTDRIVE%%_NTROOT%\private\ntos\w32\ntuser\inc\vkoem.h %_NTDRIVE%%_NTROOT%\private\genx\windows\inc\ime.w | perl vkeytbl.pl > vktbl.txt

