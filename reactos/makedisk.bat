@echo off
echo copying latest files to a:...
copy /Y bootflop.bat a:\autoexec.bat
copy /Y loaders\dos\loadros.com a:
copy /Y apps\shell\shell.bin a:
copy /Y ntoskrnl\kimage.bin a:
copy /Y services\dd\blue\blues.o a:
copy /Y services\dd\keyboard\keyboard.o a:
copy /Y services\dd\ide\ide.o a:
copy /Y services\fs\vfat\vfatfsd.o a:

