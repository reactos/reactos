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
md %ROS_INSTALL%\symbols
md %ROS_INSTALL%\system32
md %ROS_INSTALL%\system32\config
md %ROS_INSTALL%\system32\drivers
md %ROS_INSTALL%\media
md %ROS_INSTALL%\media\fonts
copy boot.bat %ROS_INSTALL%
copy bootc.lst %ROS_INSTALL%
copy aboot.bat %ROS_INSTALL%
copy system.hiv %ROS_INSTALL%\system32\config
copy loaders\dos\loadros.com %ROS_INSTALL%
copy ntoskrnl\ntoskrnl.exe %ROS_INSTALL%\system32
copy ntoskrnl\ntoskrnl.sym %ROS_INSTALL%\symbols
copy hal\halx86\hal.dll %ROS_INSTALL%\system32
copy services\fs\vfat\vfatfs.sys %ROS_INSTALL%\system32\drivers
copy services\fs\cdfs\cdfs.sys %ROS_INSTALL%\system32\drivers
copy services\fs\fs_rec\fs_rec.sys %ROS_INSTALL%\system32\drivers
copy services\fs\ms\msfs.sys %ROS_INSTALL%\system32\drivers
copy services\fs\np\npfs.sys %ROS_INSTALL%\system32\drivers
copy services\bus\acpi\acpi.sys %ROS_INSTALL%\system32\drivers
copy services\bus\isapnp\isapnp.sys %ROS_INSTALL%\system32\drivers
copy services\bus\pci\pci.sys %ROS_INSTALL%\system32\drivers
copy services\dd\ide\ide.sys %ROS_INSTALL%\system32\drivers
copy services\dd\floppy\floppy.sys %ROS_INSTALL%\system32\drivers
copy services\input\keyboard\keyboard.sys %ROS_INSTALL%\system32\drivers
copy services\input\mouclass\mouclass.sys %ROS_INSTALL%\system32\drivers
copy services\input\psaux\psaux.sys %ROS_INSTALL%\system32\drivers
copy services\dd\blue\blue.sys %ROS_INSTALL%\system32\drivers
copy services\dd\vga\miniport\vgamp.sys %ROS_INSTALL%\system32\drivers
copy services\dd\vga\display\vgaddi.dll %ROS_INSTALL%\system32\drivers
copy services\dd\vidport\vidport.sys %ROS_INSTALL%\system32\drivers
copy services\net\afd\afd.sys %ROS_INSTALL%\system32\drivers
copy services\net\dd\ne2000\ne2000.sys %ROS_INSTALL%\system32\drivers
copy services\net\ndis\ndis.sys %ROS_INSTALL%\system32\drivers
copy services\net\tcpip\tcpip.sys %ROS_INSTALL%\system32\drivers
copy services\net\wshtcpip\wshtcpip.dll %ROS_INSTALL%\system32
copy services\storage\atapi\atapi.sys %ROS_INSTALL%\system32\drivers
copy services\storage\scsiport\scsiport.sys %ROS_INSTALL%\system32\drivers
copy services\storage\cdrom\cdrom.sys %ROS_INSTALL%\system32\drivers
copy services\storage\disk\disk.sys %ROS_INSTALL%\system32\drivers
copy services\storage\class2\class2.sys %ROS_INSTALL%\system32\drivers
copy apps\system\shell\shell.exe %ROS_INSTALL%\system32
copy apps\system\winlogon\winlogon.exe %ROS_INSTALL%\system32
copy apps\system\services\services.exe %ROS_INSTALL%\system32
copy lib\advapi32\advapi32.dll %ROS_INSTALL%\system32
copy lib\crtdll\crtdll.dll %ROS_INSTALL%\system32
copy lib\fmifs\fmifs.dll %ROS_INSTALL%\system32
copy lib\gdi32\gdi32.dll %ROS_INSTALL%\system32
copy lib\kernel32\kernel32.dll %ROS_INSTALL%\system32
copy lib\msafd\msafd.dll %ROS_INSTALL%\system32
copy lib\msvcrt\msvcrt.dll %ROS_INSTALL%\system32
copy lib\ntdll\ntdll.dll %ROS_INSTALL%\system32
copy lib\secur32\secur32.dll %ROS_INSTALL%\system32
copy lib\user32\user32.dll %ROS_INSTALL%\system32
copy lib\ws2_32\ws2_32.dll %ROS_INSTALL%\system32
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
copy apps\partinfo\partinfo.exe %ROS_INSTALL%\bin
copy apps\objdir\objdir.exe %ROS_INSTALL%\bin
copy apps\mutex\mutex.exe %ROS_INSTALL%\bin
copy apps\winhello\winhello.exe %ROS_INSTALL%\bin
copy apps\sectest\sectest.exe %ROS_INSTALL%\bin
copy apps\pice\module\pice.sys %ROS_INSTALL%\system32\drivers
copy apps\pice\module\pice.sym %ROS_INSTALL%\symbols
copy apps\pice\pice.cfg %ROS_INSTALL%\symbols
copy apps\isotest\isotest.exe %ROS_INSTALL%\bin
copy media\fonts\helb____.ttf %ROS_INSTALL%\media\fonts
copy media\fonts\timr____.ttf %ROS_INSTALL%\media\fonts
copy media\nls\*.nls %ROS_INSTALL%\system32
