@echo off
echo copying latest files to a:...
copy /Y bootflop.bat a:\autoexec.bat > NUL:
echo bootflop.bat to a:\autoexec.bat
copy /Y loaders\dos\loadros.com a:\ > NUL:
echo loadros.com
copy /Y ntoskrnl\ntoskrnl.exe a:\ > NUL:
echo ntoskrnl.exe
copy /Y services\dd\ide\ide.sys a:\ > NUL:
echo ide.sys
copy /Y services\fs\vfat\vfatfsd.sys a:\ > NUL:
echo vfatfsd.sys
copy /Y services\dd\blue\blue.sys a:\drivers > NUL:
echo blue.sys
copy /Y services\dd\keyboard\keyboard.sys a:\drivers > NUL:
echo keyboard.sys
copy /Y subsys\smss\smss.exe a:\subsys > NUL:
echo smss.exe
copy /Y lib\ntdll\ntdll.dll a:\dlls > NUL:
echo lib\ntdll\ntdll.dll
copy /Y lib\kernel32\kernel32.dll a:\dlls > NUL:
echo lib\kernel32\kernel32.dll
copy /Y lib\advapi32\advapi32.dll a:\dlls > NUL:
echo lib\advapi32\advapi32.dll a:
copy /Y lib\user32\user32.dll a:\dlls > NUL:
echo lib\user32\user32.dll a:
copy /Y lib\crtdll\crtdll.dll a:\dlls > NUL:
echo lib\crtdll\crtdll.dll a:
copy /Y apps\shell\shell.exe a:\apps > NUL:
echo shell.exe



