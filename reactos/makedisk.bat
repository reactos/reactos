@echo off
echo copying latest files to a:...
copy /Y bootflop.bat a:\autoexec.bat > NUL:
echo bootflop.bat to a:\autoexec.bat
copy /Y loaders\dos\loadros.com a: > NUL:
echo loadros.com
copy /Y ntoskrnl\ntoskrnl.exe a: > NUL:
echo ntoskrnl.exe
copy /Y services\dd\ide\ide.sys a: > NUL:
echo ide.sys
copy /Y services\fs\vfat\vfatfsd.sys a: > NUL:
echo vfatfsd.sys
copy /Y services\dd\blue\blue.sys a: > NUL:
echo blue.sys
copy /Y services\dd\keyboard\keyboard.sys a: > NUL:
echo keyboard.sys
copy /Y lib\ntdll\ntdll.dll a: > NUL:
echo ntdll.dll
copy /Y lib\kernel32\kernel32.dll a: > NUL:
echo kernel32.dll
copy /Y apps\shell\shell.exe a: > NUL:
echo shell.exe
: copy /Y lib\crtdll\crtdll.dll a: > NUL:
: echo lib\crtdll\crtdll.dll a:



