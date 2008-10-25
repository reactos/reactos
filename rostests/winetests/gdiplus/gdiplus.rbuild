<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="gdiplus_winetest" type="win32cui" installbase="bin" installname="gdiplus_winetest.exe" allowwarnings="true">
	<include base="gdiplus_winetest">.</include>
	<file>brush.c</file>
	<file>font.c</file>
	<file>graphics.c</file>
	<file>graphicspath.c</file>
	<file>image.c</file>
	<file>matrix.c</file>
	<file>pathiterator.c</file>
	<file>pen.c</file>
	<file>region.c</file>
	<file>stringformat.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>gdiplus</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
