<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mshtml_winetest" type="win32cui" installbase="bin" installname="mshtml_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="mshtml_winetest">.</include>
	<file>dom.c</file>
	<file>htmldoc.c</file>
	<file>misc.c</file>
	<file>protocol.c</file>
	<file>script.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>uuid</library>
	<library>strmiids</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>user32</library>
	<library>urlmon</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
