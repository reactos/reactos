<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="browseui_winetest" type="win32cui" installbase="bin" installname="browseui_winetest.exe" allowwarnings="true" entrypoint="0">
	<include base="browseui_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>autocomplete.c</file>
	<file>testlist.c</file>
</module>
