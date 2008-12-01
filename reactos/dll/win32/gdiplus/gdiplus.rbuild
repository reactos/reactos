<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="gdiplus" type="win32dll" baseaddress="${BASEADDRESS_GDIPLUS}" installbase="system32" installname="gdiplus.dll" allowwarnings="true">
	<importlibrary definition="gdiplus.spec" />
	<include base="gdiplus">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>brush.c</file>
	<file>customlinecap.c</file>
	<file>font.c</file>
	<file>gdiplus.c</file>
	<file>graphics.c</file>
	<file>graphicspath.c</file>
	<file>image.c</file>
	<file>imageattributes.c</file>
	<file>matrix.c</file>
	<file>pathiterator.c</file>
	<file>pen.c</file>
	<file>region.c</file>
	<file>stringformat.c</file>
	<library>wine</library>
	<library>uuid</library>
	<library>shlwapi</library>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
