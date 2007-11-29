<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ole32_winetest" type="win32cui" installbase="bin" installname="ole32_winetest.exe" allowwarnings="true" entrypoint="0">
	<include base="ole32_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>clipboard.c</file>
	<file>compobj.c</file>
	<file>dragdrop.c</file>
	<file>errorinfo.c</file>
	<file>hglobalstream.c</file>
	<file>marshal.c</file>
	<file>moniker.c</file>
	<file>ole2.c</file>
	<file>propvariant.c</file>
	<file>stg_prop.c</file>
	<file>storage32.c</file>
	<file>usrmarshal.c</file>
	<file>testlist.c</file>
</module>
