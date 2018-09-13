@echo off
echo.
echo Generating DEPEND.MK...
echo.
echo NOTE: 'file not found' and 'calls undefined' msgs may be safely ignored...
set _R=..\..\..\..\..
cd debug
echo -S. -L. -i -e -D.. -D%_R%\dev\inc16 -D%_R%\dev\sdk\inc16 > .\depend.i
echo -D%_R%\dev\inc -D%_R%\dev\sdk\inc >> .\depend.i
echo -D%_R%\dev\tools\c932\inc >> .\depend.i
echo -D%_R%\win\core\inc -D..\..\inc >> .\depend.i
includes @depend.i pch.c > depend.new
sed "s/pch.obj/pch.obj .\\pch.pch/" depend.new >..\depend.new
includes @depend.i -npch.h ..\*.c ..\*.asm >>..\depend.new
cd ..
set _R=
erase depend.old
ren depend.mk depend.old
erase depend.mk
ren depend.new depend.mk
echo.
echo Old DEPEND.MK renamed to DEPEND.OLD
echo.
