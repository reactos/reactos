@echo off

IF NOT EXIST ncftp.h GOTO ERR:

cd ..
erase \temp\libncftp.zip >NUL
pkzip25.exe -add -204 -dir=current -excl=*.lib -excl=*.exe -excl=*.zip -excl=*.gz -excl=*.tar -excl=*.o -excl=*.obj -excl=*.pch -excl=*.ilk -excl=*.ncb -excl=*.opt -excl=*.pdb -excl=*.idb -excl=*.plg -excl=config.* -excl=*.so -excl=*.a -excl=Makefile -excl=core \temp\libncftp.zip libncftp\*.*
pkzip25.exe -add -204 -dir=current -excl=*.lib -excl=*.exe -excl=*.zip -excl=*.gz -excl=*.tar -excl=*.o -excl=*.obj -excl=*.pch -excl=*.ilk -excl=*.ncb -excl=*.opt -excl=*.pdb -excl=*.idb -excl=*.plg -excl=config.* -excl=*.so -excl=*.a -excl=Makefile -excl=core \temp\libncftp.zip sio\*.*
pkzip25.exe -add -204 -dir=current -excl=*.lib -excl=*.exe -excl=*.zip -excl=*.gz -excl=*.tar -excl=*.o -excl=*.obj -excl=*.pch -excl=*.ilk -excl=*.ncb -excl=*.opt -excl=*.pdb -excl=*.idb -excl=*.plg -excl=config.* -excl=*.so -excl=*.a -excl=Makefile -excl=core \temp\libncftp.zip Strn\*.*

cd libncftp
dir \temp\libncftp.zip

GOTO DONE:

:ERR
echo Please cd to the source directory and then run this script.

:DONE