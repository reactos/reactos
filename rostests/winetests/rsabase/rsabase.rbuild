<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rsabase_winetest" type="win32cui" installbase="bin" installname="rsabase_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="rsabase_winetest">.</include>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>rsabase.c</file>
	<file>testlist.c</file>
</module>
