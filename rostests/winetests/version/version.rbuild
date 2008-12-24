<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="version_winetest" type="win32cui" installbase="bin" installname="version_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="version_winetest">.</include>
	<file>info.c</file>
	<file>install.c</file>
	<file>version.rc</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>version</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
