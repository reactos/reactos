<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="comdlg32_winetest" type="win32cui" installbase="bin" installname="comdlg32_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="comdlg32_winetest">.</include>
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
