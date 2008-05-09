<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="comdlg32_winetest" type="win32cui" installbase="bin" installname="comdlg32_winetest.exe" allowwarnings="true">
	<include base="comdlg32_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>filedlg.c</file>
	<file>printdlg.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>comdlg32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
