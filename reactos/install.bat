@echo off
md c:\reactos
md c:\reactos\bin
md c:\reactos\system32
md c:\reactos\system32\config
md c:\reactos\system32\drivers
copy boot.bat c:\reactos
copy loaders\dos\loadros.com c:\reactos
copy ntoskrnl\ntoskrnl.exe c:\reactos
copy services\fs\vfat\vfatfs.sys c:\reactos
copy services\dd\ide\ide.sys c:\reactos
copy services\dd\floppy\floppy.sys c:\reactos\system32\drivers
copy services\input\keyboard\keyboard.sys c:\reactos\system32\drivers
copy services\dd\blue\blue.sys c:\reactos\system32\drivers
copy services\dd\vga\miniport\vgamp.sys c:\reactos\system32\drivers
copy services\dd\vga\display\vgaddi.dll c:\reactos\system32\drivers
copy services\dd\vidport\vidport.sys c:\reactos\system32\drivers
copy apps\system\shell\shell.exe c:\reactos\system32
copy apps\system\winlogon\winlogon.exe c:\reactos\system32
copy apps\system\services\services.exe c:\reactos\system32
copy lib\ntdll\ntdll.dll c:\reactos\system32
copy lib\kernel32\kernel32.dll c:\reactos\system32
copy lib\crtdll\crtdll.dll c:\reactos\system32
copy lib\fmifs\fmifs.dll c:\reactos\system32
copy lib\gdi32\gdi32.dll c:\reactos\system32
copy lib\advapi32\advapi32.dll c:\reactos\system32
copy lib\msvcrt\msvcrt.dll c:\reactos\system32
copy lib\user32\user32.dll c:\reactos\system32
copy apps\hello\hello.exe c:\reactos\bin
copy apps\args\args.exe c:\reactos\bin
copy apps\cat\cat.exe c:\reactos\bin
copy subsys\smss\smss.exe c:\reactos\system32
copy subsys\csrss\csrss.exe c:\reactos\system32
copy subsys\win32k\win32k.sys c:\reactos\system32\drivers
copy apps\apc\apc.exe c:\reactos\bin
copy apps\shm\shmsrv.exe c:\reactos\bin
copy apps\shm\shmclt.exe c:\reactos\bin
copy apps\lpc\lpcsrv.exe c:\reactos\bin
copy apps\lpc\lpcclt.exe c:\reactos\bin
copy apps\thread\thread.exe c:\reactos\bin
copy apps\event\event.exe c:\reactos\bin
copy apps\file\file.exe c:\reactos\bin
copy apps\pteb\pteb.exe c:\reactos\bin
copy apps\consume\consume.exe c:\reactos\bin
copy apps\vmtest\vmtest.exe c:\reactos\bin
copy apps\gditest\gditest.exe c:\reactos\bin
