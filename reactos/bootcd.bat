@echo off
set BOOTCD_DIR=..\bootcd

md %BOOTCD_DIR%
md %BOOTCD_DIR%\disk
md %BOOTCD_DIR%\disk\bootdisk
md %BOOTCD_DIR%\disk\install
md %BOOTCD_DIR%\disk\reactos
md %BOOTCD_DIR%\disk\reactos\system32

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

copy /Y txtsetup.sif %BOOTCD_DIR%\disk\install
