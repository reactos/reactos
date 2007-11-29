<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="version_winetest" type="win32cui" installbase="bin" installname="version_winetest.exe" allowwarnings="true" entrypoint="0">
	<include base="version_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>version</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>info.c</file>
	<file>install.c</file>
	<file>version.rc</file>
	<file>testlist.c</file>
</module>
