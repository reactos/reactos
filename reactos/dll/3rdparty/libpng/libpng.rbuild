<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="libpng" type="win32dll" entrypoint="0" installbase="system32" installname="libpng.dll" allowwarnings="true" crt="msvcrt">
	<define name="WIN32" />
	<define name="NDEBUG" />
	<define name="_WINDOWS" />
	<define name="PNG_BUILD_DLL" />
	<include base="libpng">.</include>
	<include base="ReactOS">include/reactos/libs/zlib</include>
	<include base="ReactOS">include/reactos/libs/libpng</include>
	<library>zlib</library>
	<file>png.c</file>
	<file>pngerror.c</file>
	<file>pngget.c</file>
	<file>pngmem.c</file>
	<file>pngpread.c</file>
	<file>pngread.c</file>
	<file>pngrio.c</file>
	<file>pngrtran.c</file>
	<file>pngrutil.c</file>
	<file>pngset.c</file>
	<file>pngtest.c</file>
	<file>pngtrans.c</file>
	<file>pngwio.c</file>
	<file>pngwrite.c</file>
	<file>pngwtran.c</file>
	<file>pngwutil.c</file>
</module>
