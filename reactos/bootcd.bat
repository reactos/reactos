@echo off
set BOOTCD_DIR=..\bootcd

md %BOOTCD_DIR%
md %BOOTCD_DIR%\disk
md %BOOTCD_DIR%\disk\bootdisk
md %BOOTCD_DIR%\disk\install
md %BOOTCD_DIR%\disk\reactos
md %BOOTCD_DIR%\disk\reactos\system32

rem copy boot files
copy /Y ntoskrnl\ntoskrnl.exe %BOOTCD_DIR%\disk\reactos
copy /Y hal\halx86\hal.dll %BOOTCD_DIR%\disk\reactos
copy /Y drivers\fs\vfat\vfatfs.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\fs\cdfs\cdfs.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\fs\ntfs\ntfs.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\dd\floppy\floppy.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\dd\blue\blue.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\input\keyboard\keyboard.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\storage\atapi\atapi.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\storage\scsiport\scsiport.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\storage\cdrom\cdrom.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\storage\disk\disk.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\storage\class2\class2.sys %BOOTCD_DIR%\disk\reactos
copy /Y lib\ntdll\ntdll.dll %BOOTCD_DIR%\disk\reactos\system32
copy /Y subsys\system\usetup\usetup.exe %BOOTCD_DIR%\disk\reactos\system32\smss.exe

rem copy install files
copy /Y txtsetup.sif %BOOTCD_DIR%\disk\install

copy /Y ntoskrnl\ntoskrnl.exe %BOOTCD_DIR%\disk\install
copy /Y hal\halx86\hal.dll %BOOTCD_DIR%\disk\install

copy /Y drivers\fs\vfat\vfatfs.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\fs\cdfs\cdfs.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\fs\ntfs\ntfs.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\fs\fs_rec\fs_rec.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\dd\beep\beep.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\dd\blue\blue.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\dd\floppy\floppy.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\dd\null\null.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\input\keyboard\keyboard.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\input\psaux\psaux.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\storage\atapi\atapi.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\storage\scsiport\scsiport.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\storage\cdrom\cdrom.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\storage\disk\disk.sys %BOOTCD_DIR%\disk\install
copy /Y drivers\storage\class2\class2.sys %BOOTCD_DIR%\disk\install

copy /Y lib\ntdll\ntdll.dll %BOOTCD_DIR%\disk\install
copy /Y lib\advapi32\advapi32.dll %BOOTCD_DIR%\disk\install
copy /Y lib\kernel32\kernel32.dll %BOOTCD_DIR%\disk\install
copy /Y lib\msvcrt\msvcrt.dll %BOOTCD_DIR%\disk\install

copy /Y subsys\smss\smss.exe %BOOTCD_DIR%\disk\install
copy /Y subsys\csrss\csrss.exe %BOOTCD_DIR%\disk\install
copy /Y subsys\system\services\services.exe %BOOTCD_DIR%\disk\install
copy /Y subsys\system\shell\shell.exe %BOOTCD_DIR%\disk\install
copy /Y subsys\system\winlogon\winlogon.exe %BOOTCD_DIR%\disk\install

rem copy temporary files
copy /Y system.hiv %BOOTCD_DIR%\disk\install
