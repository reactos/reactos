@echo off
if "%1" == "" goto NoParameter
set ROS_INSTALL=%1
goto Install
:NoParameter
set ROS_INSTALL=c:\reactos
:Install
echo on
echo Installing to %ROS_INSTALL%
@echo off

md %ROS_INSTALL%
md %ROS_INSTALL%\bin
md %ROS_INSTALL%\system32
md %ROS_INSTALL%\system32\config
md %ROS_INSTALL%\system32\drivers
md %ROS_INSTALL%\media
md %ROS_INSTALL%\media\fonts
copy boot.bat %ROS_INSTALL%
copy loaders\dos\loadros.com %ROS_INSTALL%
copy ntoskrnl\ntoskrnl.exe %ROS_INSTALL%
copy services\fs\vfat\vfatfs.sys %ROS_INSTALL%
copy services\fs\ms\msfs.sys %ROS_INSTALL%\system32\drivers
copy services\fs\np\npfs.sys %ROS_INSTALL%\system32\drivers
copy services\bus\acpi\acpi.sys %ROS_INSTALL%
copy services\bus\isapnp\isapnp.sys %ROS_INSTALL%
copy services\dd\ide\ide.sys %ROS_INSTALL%
copy services\dd\floppy\floppy.sys %ROS_INSTALL%\system32\drivers
copy services\input\keyboard\keyboard.sys %ROS_INSTALL%\system32\drivers
copy services\input\mouclass\mouclass.sys %ROS_INSTALL%\system32\drivers
copy services\input\psaux\psaux.sys %ROS_INSTALL%\system32\drivers
copy services\dd\blue\blue.sys %ROS_INSTALL%\system32\drivers
copy services\dd\vga\miniport\vgamp.sys %ROS_INSTALL%\system32\drivers
copy services\dd\vga\display\vgaddi.dll %ROS_INSTALL%\system32\drivers
copy services\dd\vidport\vidport.sys %ROS_INSTALL%\system32\drivers
copy apps\system\shell\shell.exe %ROS_INSTALL%\system32
copy apps\system\winlogon\winlogon.exe %ROS_INSTALL%\system32
copy apps\system\services\services.exe %ROS_INSTALL%\system32
copy lib\ntdll\ntdll.dll %ROS_INSTALL%\system32
copy lib\kernel32\kernel32.dll %ROS_INSTALL%\system32
copy lib\crtdll\crtdll.dll %ROS_INSTALL%\system32
copy lib\fmifs\fmifs.dll %ROS_INSTALL%\system32
copy lib\gdi32\gdi32.dll %ROS_INSTALL%\system32
copy lib\advapi32\advapi32.dll %ROS_INSTALL%\system32
copy lib\msvcrt\msvcrt.dll %ROS_INSTALL%\system32
copy lib\user32\user32.dll %ROS_INSTALL%\system32
copy apps\hello\hello.exe %ROS_INSTALL%\bin
copy apps\args\args.exe %ROS_INSTALL%\bin
copy apps\cat\cat.exe %ROS_INSTALL%\bin
copy subsys\smss\smss.exe %ROS_INSTALL%\system32
copy subsys\csrss\csrss.exe %ROS_INSTALL%\system32
copy subsys\win32k\win32k.sys %ROS_INSTALL%\system32\drivers
copy apps\apc\apc.exe %ROS_INSTALL%\bin
copy apps\shm\shmsrv.exe %ROS_INSTALL%\bin
copy apps\shm\shmclt.exe %ROS_INSTALL%\bin
copy apps\lpc\lpcsrv.exe %ROS_INSTALL%\bin
copy apps\lpc\lpcclt.exe %ROS_INSTALL%\bin
copy apps\thread\thread.exe %ROS_INSTALL%\bin
copy apps\event\event.exe %ROS_INSTALL%\bin
copy apps\file\file.exe %ROS_INSTALL%\bin
copy apps\pteb\pteb.exe %ROS_INSTALL%\bin
copy apps\consume\consume.exe %ROS_INSTALL%\bin
copy apps\vmtest\vmtest.exe %ROS_INSTALL%\bin
copy apps\gditest\gditest.exe %ROS_INSTALL%\bin
copy apps\mstest\msserver.exe %ROS_INSTALL%\bin
copy apps\mstest\msclient.exe %ROS_INSTALL%\bin
copy apps\nptest\npserver.exe %ROS_INSTALL%\bin
copy apps\nptest\npclient.exe %ROS_INSTALL%\bin
copy apps\atomtest\atomtest.exe %ROS_INSTALL%\bin
copy media\fonts\helb____.ttf %ROS_INSTALL%\media\fonts
copy media\fonts\timr____.ttf %ROS_INSTALL%\media\fonts
