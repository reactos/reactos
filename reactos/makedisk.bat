@echo off
echo copying latest files to a:...
copy /Y bootflop.bat a:\autoexec.bat
copy /Y loaders\dos\loadros.com a:
copy /Y apps\shell\shell.exe a:
copy /Y ntoskrnl\ntoskrnl.exe a:
copy /Y services\dd\blue\blue.sys a:
copy /Y services\dd\keyboard\keyboard.sys a:
copy /Y services\dd\ide\ide.sys a:
copy /Y services\fs\vfat\vfatfsd.sys a:
copy /Y lib\ntdll\ntdll.dll a:
: copy /Y lib\crtdll\crtdll.dll a:
