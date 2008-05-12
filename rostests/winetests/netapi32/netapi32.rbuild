<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="netapi32_winetest" type="win32cui" installbase="bin" installname="netapi32_winetest.exe" allowwarnings="true">
	<include base="netapi32_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>access.c</file>
	<file>apibuf.c</file>
	<file>ds.c</file>
	<file>wksta.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
